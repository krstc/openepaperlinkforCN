/**
 * @file ESP32_EPD_BLE.ino
 * @brief 基于ESP32和电子纸显示屏(EPD)的BLE图片传输与自动播放系统
 * 
 * 功能概述：
 * 1. 通过BLE接收图片数据（二进制文件），保存到SPIFFS文件系统
 * 2. 支持命令控制（列表、删除、显示、格式化、自动播放等）
 * 3. 支持深度睡眠节能模式，唤醒后自动检查并显示下一张图片（自动播放）
 * 4. 使用NimBLE库（低功耗蓝牙）
 * 
 * 硬件连接：根据所选EPD型号（SE0398NZ07A0等）进行SPI引脚连接
 * 
 * 通信协议：
 * - 服务UUID：0000ffe0-0000-1000-8000-00805f9b34fb
 * - 特征值：
 *   - 0000ffe1：写入图片数据（二进制流）
 *   - 0000ffe2：写入命令/读取响应（字符串，以换行符结束）
 *   - 0000ffe4：只读，返回EPD型号
 *   - 0000ffe5：只读，返回屏幕宽度
 *   - 0000ffe6：只读，返回屏幕高度
 *   - 0000ffe7：只读，返回颜色模式
 * 
 * 命令列表（通过特征0000ffe2发送，响应通过该特征通知）：
 * - FORMAT                 : 格式化SPIFFS，重启
 * - LIST [ALL]             : 列出所有图片文件（IMG_*.bin）或所有文件
 * - DELETE <filename>      : 删除指定文件
 * - SHOW <filename>        : 显示指定图片文件
 * - AUTO <间隔秒数> <模式> : 启动自动播放，模式0=顺序，1=倒序，2=随机
 * - STATUS                 : 显示SPIFFS使用情况、图片数量、下一个可用索引
 * - HELP                   : 显示帮助
 * - RESET                  : 重启ESP32
 * - SLEEP                  : 立即进入深度睡眠
 * 
 * 自动播放机制：
 * - 使用RTC_DATA_ATTR变量在深度睡眠期间保持状态（启用标志、间隔、模式、当前索引、虚拟时间计数器）
 * - 每次唤醒时增加虚拟时间计数器（模拟睡眠经过的时间），达到间隔后自动显示下一张图片
 * - 唤醒后若无BLE连接，则检查自动播放条件，显示图片后再次进入深度睡眠
 * - 若有BLE连接，则进入交互模式，超时无操作后断开连接并睡眠
 * 
 * 文件命名规则：IMG_XXXX.bin，其中XXXX为4位数字序号（从0001开始自动分配）
 * 文件内容：原始屏幕像素数据（二进制），大小等于 epd.allScreenBytes
 * 
 * 注意：本代码为特定EPD库（SE0398NZ07A0等）编写，需根据实际型号包含对应头文件
 */

#include <SPI.h>
#include "EPD.h"
#include "EL044TS2.h"
#include "EL073TF1.h"
#include "EL081TS2.h"
#include "HE583A04A1.h"
#include "HSE097SE.h"
#include "WFT0371CZ78.h"
#include "SE0398NZ07A0.h"
#include "EPD_3in98g.h"

#include <NimBLEDevice.h>
#include "SPIFFS.h"

// 根据实际使用的EPD型号创建对象（此处为EPD_3in98g）
EPD_3in98g epd;

// ---------- BLE UUID 定义 ----------
#define SERVICE_UUID "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"    // 图片数据写入特征
#define CHARACTERISTIC_COMMAND "0000ffe2-0000-1000-8000-00805f9b34fb" // 命令/响应特征
#define CHARACTERISTIC_MODEL "0000ffe4-0000-1000-8000-00805f9b34fb"    // 只读：型号
#define CHARACTERISTIC_WIDTH "0000ffe5-0000-1000-8000-00805f9b34fb"    // 只读：宽度
#define CHARACTERISTIC_HEIGHT "0000ffe6-0000-1000-8000-00805f9b34fb"   // 只读：高度
#define CHARACTERISTIC_COLOR "0000ffe7-0000-1000-8000-00805f9b34fb"    // 只读：颜色模式

// ---------- 超时定义（单位：毫秒）----------
const unsigned long INACTIVITY_TIMEOUT = 60000; // 连接后无任何BLE通信（命令或数据）的超时，断开连接
const unsigned long TRANSFER_TIMEOUT = 10000;   // 图片数据传输过程中，若超过此时间未收到新数据，视为中断，断开连接并删除不完整文件
const unsigned long DEEPSLEEP_TIME = 10000;      // 深度睡眠持续时间（毫秒），唤醒后检查自动播放
const unsigned long WAIT_TIMEOUT = 10000;         // 广播后等待客户端连接的最长时间（毫秒），超时则进入睡眠

// ---------- 自动播放参数（存储在RTC内存中，深度睡眠后保持）----------
RTC_DATA_ATTR bool autoPlayEnabled = false;       // 是否处于自动播放模式
RTC_DATA_ATTR unsigned long autoPlayInterval = 0; // 自动播放间隔（秒）
RTC_DATA_ATTR int autoPlayMode = 0;               // 0=顺序,1=倒序,2=随机
RTC_DATA_ATTR int autoPlayCurrentIndex = 0;       // 当前显示的文件在列表中的索引
RTC_DATA_ATTR unsigned long totalElapsedSeconds = 0;   // 虚拟时间计数器（累计经过的秒数，每次唤醒增加睡眠时长）
RTC_DATA_ATTR unsigned long lastAutoDisplayTime = 0;   // 上一次显示图片时的虚拟时间戳

// ---------- 全局变量 ----------
unsigned int dataIndex = 0;         // 当前接收图片数据的字节偏移量
bool dataReceived = false;          // 是否完整接收完一张图片
bool isConnected = false;           // BLE客户端是否已连接
bool isAdvertising = true;          // 是否正在广播（仅用于调试）
unsigned long lastActionTime = millis(); // 最后一次BLE通信（数据或命令）的时间戳
String autoPlayFileList[100];       // 存储所有图片文件路径的列表（最大100个）
int autoPlayFileCount = 0;          // 实际图片文件数量
NimBLEServer *pServer = nullptr;            // BLE服务器对象指针
NimBLECharacteristic *pCommandCharacteristic = nullptr; // 命令特征指针，用于发送响应

// 文件写入相关
File currentFile;           // 当前正在写入的文件对象
String currentFileName;     // 当前正在写入的文件名（如"/IMG_0001.bin"）

// ---------- 函数声明 ----------
void enterDeepSleep();
void checkTimeoutsAndDisconnect(); // 连接状态下的超时检测（仅断开连接，不深度睡眠）
void closeAndDeleteCurrentFile();  // 关闭并删除未完成接收的文件（传输失败时调用）
void startNewFile();               // 创建新文件用于写入图片数据
void handleCommand(const String &cmdStr);
void formatSPIFFS();
void listFiles(bool listAll = false);
void deleteFile(const String &filename);
void showFile(const String &filename);
void statusCommand();
void startAutoPlay(unsigned long interval, int mode);
void scanImageFiles();             // 扫描SPIFFS中所有IMG_*.bin文件，填充autoPlayFileList
void sendCommandResponse(const String &response);
void helpCommand();
int getNextAvailableIndex();       // 获取下一个可用的文件序号（0001-9999）
bool displayImageFromSPIFFS(const String &filepath); // 从SPIFFS读取图片文件并显示到EPD
void autoPlayCheckAndShow();       // 检查自动播放条件（虚拟时间间隔），满足则显示下一张图片

// ---------- BLE 回调类（NimBLE 风格）----------
/**
 * @brief BLE服务器事件回调
 * 处理连接、断开连接、MTU变更等事件
 */
class MyServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
  {
    // 更新连接参数（最小12ms，最大24ms，延迟0，超时200ms），优化传输性能
    pServer->updateConnParams(connInfo.getConnHandle(), 12, 24, 0, 200);
    isConnected = true;
    isAdvertising = false;
    lastActionTime = millis();
    Serial.println("设备已连接");
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
  {
    isConnected = false;
    // 如果连接断开时还有未完成的文件传输（已接收部分数据但未达到完整大小），则删除该不完整文件
    if (dataIndex > 0 && dataIndex < epd.allScreenBytes && currentFile)
    {
      Serial.println("连接断开，删除未完成的文件");
      closeAndDeleteCurrentFile();
    }
    dataIndex = 0;
    dataReceived = false;
    lastActionTime = millis();
    Serial.println("设备已断开连接");
  }

  void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override
  {
    Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
  }
};

/**
 * @brief 只读特征的回调（型号、宽度、高度、颜色）
 * 当客户端读取时，直接返回预设的字符串值
 */
class EPDInfoCallbacks : public NimBLECharacteristicCallbacks
{
public:
  explicit EPDInfoCallbacks(const String &info) : infoValue(info) {}
  void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    pCharacteristic->setValue(infoValue);
    lastActionTime = millis(); // 更新活动时间
    Serial.printf("客户端读取EPD信息: %s\n", infoValue.c_str());
  }

private:
  String infoValue;
};

/**
 * @brief 图片数据特征（0000ffe1）的回调
 * 客户端通过write写入二进制图片数据，每次写入的数据块会追加到当前文件
 */
class DataCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    std::string rxValue = pCharacteristic->getValue();
    int dataLength = rxValue.length();

    // 如果是第一个数据块（dataIndex == 0），则创建新文件
    if (dataIndex == 0)
    {
      startNewFile();
      if (!currentFile)
      {
        Serial.println("错误：无法创建新文件，终止传输");
        if (pCommandCharacteristic && isConnected)
        {
          pCommandCharacteristic->setValue("ERROR: no free space");
          pCommandCharacteristic->notify();
        }
        return;
      }
    }

    if (currentFile)
    {
      // 将接收到的数据块写入文件
      size_t written = currentFile.write((const uint8_t *)rxValue.c_str(), dataLength);
      if (written != dataLength)
      {
        Serial.println("写入文件失败！");
        closeAndDeleteCurrentFile(); // 写入失败则删除不完整文件
        dataIndex = 0;
        return;
      }
      dataIndex += dataLength;
      lastActionTime = millis(); // 更新活动时间
      Serial.printf("已接收并写入: %d / %d 字节\n", dataIndex, epd.allScreenBytes);

      // 如果接收完成（达到预期总字节数），关闭文件并标记完成
      if (dataIndex >= epd.allScreenBytes)
      {
        currentFile.close();
        dataReceived = true;
        Serial.println("接收到完整数据，文件已保存: " + currentFileName);
      }
    }
    else
    {
      Serial.println("错误：文件未打开");
    }
  }
};

/**
 * @brief 命令特征（0000ffe2）的回调
 * 客户端发送字符串命令，执行相应操作并发送响应（通过notify）
 */
class CommandCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
  {
    std::string rxValue = pCharacteristic->getValue();
    String cmdStr = String(rxValue.c_str());
    cmdStr.trim();
    Serial.println("收到命令: " + cmdStr);
    lastActionTime = millis(); // 更新活动时间
    handleCommand(cmdStr);     // 处理命令
  }
};

// ---------- SPIFFS 文件管理函数 ----------

/**
 * @brief 获取下一个可用的文件序号（从1到9999中第一个不存在的序号）
 * @return 可用的序号（1-9999），若0表示已满（最多9999个文件）
 */
int getNextAvailableIndex()
{
  for (int idx = 1; idx <= 9999; idx++)
  {
    char filename[32];
    sprintf(filename, "/IMG_%04d.bin", idx);
    if (!SPIFFS.exists(filename))
    {
      return idx;
    }
  }
  return 0; // 序号已满
}

/**
 * @brief 创建一个新文件用于写入图片数据
 * 自动分配下一个可用序号，文件名格式为 /IMG_XXXX.bin
 * 同时检查剩余空间是否足够存储一张完整图片
 */
void startNewFile()
{
  if (currentFile)
    currentFile.close();

  // 检查剩余空间是否足够存放一张图片
  size_t freeBytes = SPIFFS.totalBytes() - SPIFFS.usedBytes();
  if (freeBytes < epd.allScreenBytes)
  {
    Serial.println("错误：剩余空间不足，无法创建新文件");
    currentFileName = "";
    return;
  }

  int newIdx = getNextAvailableIndex();
  if (newIdx == 0)
  {
    Serial.println("错误：没有可用的文件序号（最多9999个文件）");
    return;
  }
  char filename[32];
  sprintf(filename, "/IMG_%04d.bin", newIdx);
  currentFileName = String(filename);
  currentFile = SPIFFS.open(currentFileName, FILE_WRITE);
  if (!currentFile)
  {
    Serial.println("打开文件失败: " + currentFileName);
    currentFileName = "";
    return;
  }
  Serial.println("创建新文件: " + currentFileName);
}

/**
 * @brief 关闭并删除当前未完成的文件（传输失败或连接断开时调用）
 */
void closeAndDeleteCurrentFile()
{
  if (currentFile)
  {
    currentFile.close();
  }
  if (currentFileName.length() > 0 && SPIFFS.exists(currentFileName))
  {
    SPIFFS.remove(currentFileName);
    Serial.println("删除未完成文件: " + currentFileName);
  }
  currentFileName = "";
}

// ---------- 显示图片（从 SPIFFS 读取并写入 epd 缓冲区）----------

/**
 * @brief 从SPIFFS读取指定图片文件，并将其数据发送给EPD驱动进行显示
 * @param filepath 文件路径（如 "/IMG_0001.bin"）
 * @return true 显示成功，false 失败（文件不存在、大小不匹配等）
 */
bool displayImageFromSPIFFS(const String &filepath)
{
  Serial.println("开始显示图片: " + filepath);
  
  File file = SPIFFS.open(filepath, "r");
  if (!file)
  {
    Serial.println("无法打开位图文件: " + filepath);
    return false;
  }
  
  size_t fileSize = file.size();
  Serial.printf("文件大小: %u 字节, 预期: %u 字节\n", fileSize, epd.allScreenBytes);
  
  // 检查文件大小是否与屏幕所需数据量一致
  if (fileSize != epd.allScreenBytes)
  {
    Serial.printf("文件大小 (%u) 与预期 (%u) 不匹配\n", fileSize, epd.allScreenBytes);
    file.close();
    return false;
  }

  unsigned int localDataIndex = 0;
  const size_t CHUNK_SIZE = 512;
  uint8_t buffer[CHUNK_SIZE];
  int chunkCount = 0;

  Serial.println("开始分块读取文件...");
  
  // 分块读取文件，逐块调用epd.writeToRAM将数据写入EPD的显示缓冲区
  while (file.available())
  {
    unsigned int bytesRead = file.read(buffer, CHUNK_SIZE);
    if (bytesRead == 0)
      break;
      
    chunkCount++;
    Serial.printf("读取第%d块数据: %u 字节\n", chunkCount, bytesRead);
    
    // 注意：epd.writeToRAM期望接收String类型，这里将buffer转换为String（适用于该EPD库）
    // 实际库内部会将String转为二进制数据，此处不修改原有逻辑
    String chunk((char *)buffer, bytesRead);
    epd.writeToRAM(chunk, &localDataIndex, bytesRead);
    
    Serial.printf("第%d块数据处理完成，当前累计字节数: %u\n", chunkCount, localDataIndex);
  }
  file.close();

  Serial.printf("文件读取完成，共读取 %d 块数据\n", chunkCount);
  Serial.printf("实际传输 %u 字节，预期 %u 字节\n", localDataIndex, epd.allScreenBytes);
  
  if (localDataIndex != epd.allScreenBytes)
  {
    Serial.printf("错误: 实际传输 %u 字节，预期 %u 字节\n", localDataIndex, epd.allScreenBytes);
    return false;
  }

  Serial.println("图像数据传输完成");
  return true;
}

// ---------- 命令处理 ----------

/**
 * @brief 通过命令特征发送响应字符串（自动添加换行符）
 * @param response 响应内容（不含换行符）
 */
void sendCommandResponse(const String &response)
{
  if (pCommandCharacteristic && isConnected)
  {
    String msg = response;
    if (!msg.endsWith("\n"))
      msg += "\n";
    pCommandCharacteristic->setValue(msg.c_str());
    pCommandCharacteristic->notify();
    Serial.println("响应已发送: " + msg);
  }
  else
  {
    Serial.println("无法发送响应（未连接或无特征）");
  }
}

/**
 * @brief 处理从客户端收到的命令字符串
 * @param cmdStr 原始命令（已去除首尾空白）
 */
void handleCommand(const String &cmdStr)
{
  String upperCmd = cmdStr;
  upperCmd.toUpperCase();

  if (upperCmd == "FORMAT")
  {
    sendCommandResponse("FORMAT Start.Rebootting...");
    formatSPIFFS();
  }
  else if (upperCmd.startsWith("LIST"))
  {
    bool listAll = false;
    if (cmdStr.length() > 4)
    {
      String param = cmdStr.substring(4);
      param.trim();
      param.toUpperCase();
      if (param == "ALL")
        listAll = true;
    }
    listFiles(listAll);
  }
  else if (upperCmd == "DELETE")
  {
    sendCommandResponse("DELETE_ERROR: missing filename");
  }
  else if (upperCmd.startsWith("DELETE "))
  {
    String filename = cmdStr.substring(7);
    filename.trim();
    if (filename.length() == 0)
    {
      sendCommandResponse("DELETE_ERROR: missing filename");
      return;
    }
    deleteFile(filename);
  }
  else if (upperCmd == "SHOW")
  {
    sendCommandResponse("SHOW_ERROR: missing filename");
  }
  else if (upperCmd.startsWith("SHOW "))
  {
    String filename = cmdStr.substring(5);
    filename.trim();
    if (filename.length() == 0)
    {
      sendCommandResponse("SHOW_ERROR: missing filename");
      return;
    }
    showFile(filename);
  }
  else if (upperCmd == "AUTO")
  {
    sendCommandResponse("AUTOPLAY_ERROR: need interval and mode");
  }
  else if (upperCmd.startsWith("AUTO "))
  {
    // 命令格式：AUTO <间隔秒数> <模式>
    int firstSpace = cmdStr.indexOf(' ', 5);
    if (firstSpace == -1)
    {
      sendCommandResponse("AUTOPLAY_ERROR: need interval and mode");
      return;
    }
    String intervalStr = cmdStr.substring(5, firstSpace);
    String modeStr = cmdStr.substring(firstSpace + 1);
    intervalStr.trim();
    modeStr.trim();
    if (intervalStr.length() == 0 || modeStr.length() == 0)
    {
      sendCommandResponse("AUTOPLAY_ERROR: need interval and mode");
      return;
    }
    unsigned long interval = intervalStr.toInt();
    int mode = modeStr.toInt();
    // 参数范围限制：间隔30秒到86400秒（1天）
    if (interval < 30)
      interval = 30;
    if (interval > 86400)
      interval = 86400;
    if (mode < 0 || mode > 2)
      mode = 0;
    startAutoPlay(interval, mode);
  }
  else if (upperCmd == "STATUS")
  {
    statusCommand();
  }
  else if (upperCmd == "HELP")
  {
    helpCommand();
  }
  else if (upperCmd == "RESET")
  {
    sendCommandResponse("RESET_OK");
    delay(500);
    esp_restart();
  }
  else if (upperCmd == "SLEEP")
  {
    sendCommandResponse("SLEEP_OK");
    delay(500);
    if (currentFile)
      currentFile.close();
    if (dataIndex > 0 && dataIndex < epd.allScreenBytes && currentFileName.length() > 0)
    {
      closeAndDeleteCurrentFile();
    }
    esp_deep_sleep_start();
  }
  else
  {
    sendCommandResponse("UNKNOWN_COMMAND");
  }
}

/**
 * @brief 发送帮助信息
 */
void helpCommand()
{
  String helpText = "有效命令：\n";
  helpText += "FORMAT,LIST,DELETE,SHOW,AUTO,STATUS,HELP,RESET,SLEEP\n";
  sendCommandResponse(helpText);
}

/**
 * @brief 格式化SPIFFS文件系统，成功后重启设备
 */
void formatSPIFFS()
{
  Serial.println("准备格式化 SPIFFS...");
  lastActionTime = millis();
  if (currentFile)
    currentFile.close();

  Serial.println("开始格式化...");
  if (SPIFFS.format())
  {
    Serial.println("格式化成功，正在重启设备...");
    autoPlayEnabled = false; // 清除自动播放状态
    delay(100);
    esp_restart();
  }
  else
  {
    Serial.println("格式化失败！");
    if (pServer)
    {
      pServer->startAdvertising();
    }
    sendCommandResponse("FORMAT_FAILED");
  }
}

/**
 * @brief 列出文件列表
 * @param listAll true则列出所有文件，false只列出图片文件（IMG_*.bin）
 */
void listFiles(bool listAll)
{
  File root = SPIFFS.open("/");
  if (!root)
  {
    sendCommandResponse("LIST_ERROR: cannot open root");
    return;
  }
  File file = root.openNextFile();
  int fileCount = 0;
  String list = listAll ? "ALL FILES:\n" : "IMAGE FILES:\n";
  while (file)
  {
    String name = file.name();
    size_t size = file.size();
    if (listAll)
    {
      list += "/" + name + " (" + String(size) + " B)\n";
      fileCount++;
    }
    else
    {
      if (name.startsWith("IMG_") && name.endsWith(".bin"))
      {
        list += "/" + name + "\n";
        fileCount++;
      }
    }
    file = root.openNextFile();
  }
  if (fileCount == 0)
    list += listAll ? "No files found." : "No image files found.";
  sendCommandResponse(list);
}

/**
 * @brief 删除指定文件
 * @param filename 文件名（可带或不带前导斜杠）
 */
void deleteFile(const String &filename)
{
  String path = filename;
  if (!path.startsWith("/"))
    path = "/" + path;
  if (SPIFFS.exists(path))
  {
    if (SPIFFS.remove(path))
    {
      sendCommandResponse("DELETE_OK: " + filename);
      Serial.println("已删除文件: " + path);
    }
    else
    {
      sendCommandResponse("DELETE_FAILED: " + filename);
    }
  }
  else
  {
    sendCommandResponse("FILE_NOT_FOUND: " + filename);
  }
}

/**
 * @brief 显示指定图片文件
 * @param filename 文件名（可带或不带前导斜杠）
 */
void showFile(const String &filename)
{
  String path = filename;
  if (!path.startsWith("/"))
    path = "/" + path;
  if (!SPIFFS.exists(path))
  {
    sendCommandResponse("SHOW_ERROR: file not found");
    return;
  }
  Serial.println("显示图片: " + path);
  if (displayImageFromSPIFFS(path))
  {
    epd.autoSequence(); // 刷新EPD显示（该函数通常使能自动模式并更新屏幕）
    sendCommandResponse("SHOW_OK: " + filename);
  }
  else
  {
    sendCommandResponse("SHOW_ERROR: display failed");
  }
  lastActionTime = millis();
}

/**
 * @brief 发送状态信息：总容量、已用、剩余、图片数量、下一个可用索引
 */
void statusCommand()
{
  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  size_t free = total - used;

  File root = SPIFFS.open("/");
  int imgCount = 0;
  File file = root.openNextFile();
  while (file)
  {
    String name = file.name();
    if (name.startsWith("IMG_") && name.endsWith(".bin"))
      imgCount++;
    file = root.openNextFile();
  }

  int nextIdx = getNextAvailableIndex();
  String status = "Total: " + String(total) + " B\n";
  status += "Used: " + String(used) + " B\n";
  status += "Free: " + String(free) + " B\n";
  status += "Image files: " + String(imgCount) + "\n";
  status += "Next available index: " + String(nextIdx);
  sendCommandResponse(status);
}

// ---------- 自动播放功能 ----------

/**
 * @brief 扫描SPIFFS根目录，收集所有IMG_*.bin文件路径，存入autoPlayFileList
 */
void scanImageFiles()
{
  autoPlayFileCount = 0;
  File root = SPIFFS.open("/");
  if (!root)
    return;
  File file = root.openNextFile();
  while (file && autoPlayFileCount < 100)
  {
    String name = file.name();
    if (name.startsWith("IMG_") && name.endsWith(".bin"))
    {
      autoPlayFileList[autoPlayFileCount++] = "/" + name;
    }
    file = root.openNextFile();
  }
  Serial.printf("扫描到 %d 个图片文件\n", autoPlayFileCount);
}

/**
 * @brief 根据当前索引和模式计算下一张图片的索引
 * @param current 当前索引
 * @param mode 0=顺序+1,1=倒序-1,2=随机
 * @param total 总文件数
 * @return 下一张索引，若total==0则返回-1
 */
int getNextIndex(int current, int mode, int total)
{
  if (total == 0)
    return -1;
  switch (mode)
  {
  case 0:
    return (current + 1) % total;
  case 1:
    return (current - 1 + total) % total;
  case 2:
    return random(total);
  default:
    return (current + 1) % total;
  }
}

/**
 * @brief 启动自动播放模式
 * @param interval 间隔秒数（已确保在30~86400之间）
 * @param mode 播放模式：0顺序，1倒序，2随机
 */
void startAutoPlay(unsigned long interval, int mode)
{
  scanImageFiles();
  if (autoPlayFileCount == 0)
  {
    sendCommandResponse("AUTOPLAY_ERROR: no image files found");
    return;
  }
  autoPlayEnabled = true;
  autoPlayInterval = interval;
  autoPlayMode = mode;
  autoPlayCurrentIndex = 0;
  // 重置虚拟计时器，使得下一次唤醒时立即显示第一张图片（lastAutoDisplayTime设为0，而totalElapsedSeconds从0开始）
  totalElapsedSeconds = 0;
  lastAutoDisplayTime = 0;

  // 立即显示第一张图片
  if (autoPlayFileCount > 0)
  {
    String firstFile = autoPlayFileList[autoPlayCurrentIndex];
    Serial.println("自动播放显示: " + firstFile);
    if (displayImageFromSPIFFS(firstFile))
    {
      epd.autoSequence();
    }
    else
    {
      Serial.println("自动播放显示失败");
    }
    lastAutoDisplayTime = totalElapsedSeconds;
  }

  sendCommandResponse("AUTOPLAY_START interval=" + String(interval) + " mode=" + String(mode));
}

/**
 * @brief 检查自动播放条件，若虚拟时间到达间隔则显示下一张图片
 * 该函数在每次唤醒后调用（setup中），通过虚拟计时器模拟睡眠经过的时间
 */
void autoPlayCheckAndShow()
{
  if (!autoPlayEnabled || autoPlayFileCount == 0)
    return;

  // 每次唤醒后增加虚拟时间计数器（DEEPSLEEP_TIME毫秒转为秒）
  totalElapsedSeconds += DEEPSLEEP_TIME / 1000;

  // 判断是否到达显示间隔
  if (totalElapsedSeconds - lastAutoDisplayTime >= autoPlayInterval)
  {
    // 计算下一张图片索引
    int nextIdx = getNextIndex(autoPlayCurrentIndex, autoPlayMode, autoPlayFileCount);
    if (nextIdx < 0)
    {
      autoPlayEnabled = false;
      Serial.println("自动播放终止：无有效图片");
      return;
    }
    autoPlayCurrentIndex = nextIdx;
    String fileToShow = autoPlayFileList[autoPlayCurrentIndex];
    if (fileToShow.length() == 0)
    {
      // 防御性代码：若文件名为空，重新扫描一次
      Serial.println("错误：文件名为空，重新扫描");
      scanImageFiles();
      if (autoPlayCurrentIndex >= autoPlayFileCount)
        autoPlayCurrentIndex = 0;
      if (autoPlayFileCount == 0)
      {
        autoPlayEnabled = false;
        return;
      }
      fileToShow = autoPlayFileList[autoPlayCurrentIndex];
    }
    Serial.printf("自动播放显示: %s (间隔 %lu 秒)\n", fileToShow.c_str(), autoPlayInterval);
    if (displayImageFromSPIFFS(fileToShow))
    {
      epd.autoSequence();
    }
    else
    {
      Serial.println("自动播放显示失败，可能文件损坏");
    }
    lastAutoDisplayTime = totalElapsedSeconds;
  }
}

// ---------- 连接状态下的超时检测（仅断开连接，不深度睡眠）----------

/**
 * @brief 在连接状态下检查空闲超时和传输中断超时，超时则主动断开连接
 * 该函数在交互循环中周期性调用
 */
void checkTimeoutsAndDisconnect()
{
  unsigned long currentTime = millis();
  unsigned long inactiveDuration = currentTime - lastActionTime;

  // 无任何BLE通信（数据或命令）超过 INACTIVITY_TIMEOUT，断开连接
  if (isConnected && inactiveDuration > INACTIVITY_TIMEOUT)
  {
    Serial.println("连接后无操作超时，断开连接");
    if (pServer)
      pServer->disconnect(0);
    isConnected = false;
    return;
  }

  // 数据传输过程中，已接收部分数据但未完成，且超过 TRANSFER_TIMEOUT 未收到新数据，视为中断
  if (isConnected && dataIndex > 0 && dataIndex < epd.allScreenBytes && inactiveDuration > TRANSFER_TIMEOUT)
  {
    Serial.println("数据传输中断超时，断开连接并删除未完成文件");
    closeAndDeleteCurrentFile();
    dataIndex = 0;
    if (pServer)
      pServer->disconnect(0);
    isConnected = false;
  }
}

// ---------- 深度睡眠 ----------

/**
 * @brief 进入深度睡眠，唤醒定时器设置为 DEEPSLEEP_TIME 毫秒
 * 进入睡眠前关闭所有打开的文件，避免损坏文件系统
 */
void enterDeepSleep()
{
  Serial.printf("进入深度睡眠 %d 秒...", DEEPSLEEP_TIME / 1000);
  if (currentFile)
    currentFile.close();
  delay(100);
  esp_sleep_enable_timer_wakeup(DEEPSLEEP_TIME * 1000ULL);
  esp_deep_sleep_start();
}

// ---------- 初始化 ----------

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32已启动 (NimBLE - 节能模式)");

  // 1. 挂载 SPIFFS 文件系统
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS挂载失败，尝试格式化...");
    if (SPIFFS.format())
    {
      Serial.println("格式化成功，重启");
      esp_restart();
    }
    else
    {
      Serial.println("格式化失败，停止运行");
      while (1)
      {
        delay(100);
      }
    }
  }

  // 2. 如果启用了自动播放模式，则扫描图片文件并检查是否需要显示图片
  //    注意：此处在BLE初始化之前执行，使得唤醒后能快速显示图片然后再次睡眠，不占用BLE广播时间
  if (autoPlayEnabled)
  {
    scanImageFiles();
    if (autoPlayFileCount == 0)
    {
      Serial.println("自动播放已启用但无图片文件，自动关闭");
      autoPlayEnabled = false;
    }
    else
    {
      // 确保当前索引有效（防止文件列表变化导致越界）
      if (autoPlayCurrentIndex >= autoPlayFileCount)
        autoPlayCurrentIndex = 0;
      autoPlayCheckAndShow(); // 根据虚拟时间判断是否显示下一张
    }
  }

  // 3. 初始化 BLE 服务
  NimBLEDevice::init("ESP32_EPD_Display");
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // 数据特征（只写）
  NimBLECharacteristic *pDataChar = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      NIMBLE_PROPERTY::WRITE);
  pDataChar->setCallbacks(new DataCallbacks());

  // 命令特征（写 + 通知）
  pCommandCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_COMMAND,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  pCommandCharacteristic->setCallbacks(new CommandCallbacks());

  // 只读特征：型号、宽度、高度、颜色
  NimBLECharacteristic *pModelChar = pService->createCharacteristic(
      CHARACTERISTIC_MODEL,
      NIMBLE_PROPERTY::READ);
  pModelChar->setValue(epd.model);
  pModelChar->setCallbacks(new EPDInfoCallbacks(epd.model));

  NimBLECharacteristic *pWidthChar = pService->createCharacteristic(
      CHARACTERISTIC_WIDTH,
      NIMBLE_PROPERTY::READ);
  String widthStr = String(epd.width);
  pWidthChar->setValue(widthStr);
  pWidthChar->setCallbacks(new EPDInfoCallbacks(widthStr));

  NimBLECharacteristic *pHeightChar = pService->createCharacteristic(
      CHARACTERISTIC_HEIGHT,
      NIMBLE_PROPERTY::READ);
  String heightStr = String(epd.height);
  pHeightChar->setValue(heightStr);
  pHeightChar->setCallbacks(new EPDInfoCallbacks(heightStr));

  NimBLECharacteristic *pColorChar = pService->createCharacteristic(
      CHARACTERISTIC_COLOR,
      NIMBLE_PROPERTY::READ);
  String colorStr = String(epd.colorMode);
  pColorChar->setValue(colorStr);
  pColorChar->setCallbacks(new EPDInfoCallbacks(colorStr));

  pService->start();

  // 4. 开始广播，等待客户端连接
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("ESP32 BLE EPD");
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();
  Serial.println("BLE广播已启动，等待连接...");

  // 5. 等待连接窗口（WAIT_TIMEOUT毫秒），超时则进入深度睡眠
  unsigned long startWait = millis();
  while (millis() - startWait < WAIT_TIMEOUT)
  {
    if (isConnected)
      break;
    delay(10);
  }

  if (isConnected)
  {
    // 已连接，进入交互模式循环
    Serial.println("客户端已连接，进入交互模式");
    while (isConnected)
    {
      checkTimeoutsAndDisconnect(); // 超时则断开连接，并将isConnected设为false
      if (dataReceived)
      {
        // 完整文件接收完成，立即显示图片
        if (currentFile)
          currentFile.close();
        Serial.println("开始显示图片: " + currentFileName);
        if (displayImageFromSPIFFS(currentFileName))
        {
          epd.autoSequence();
        }
        dataIndex = 0;
        dataReceived = false;
        currentFileName = "";
        lastActionTime = millis(); // 显示图片后重置活动时间
      }
      delay(10);
    }
    // 连接已断开，清理状态后进入深度睡眠
    Serial.println("连接已断开，将进入深度睡眠");
    // 如果存在未完成的传输文件，删除之
    if (dataIndex > 0 && dataIndex < epd.allScreenBytes && currentFileName.length() > 0)
    {
      closeAndDeleteCurrentFile();
    }
    dataIndex = 0;
    dataReceived = false;
    delay(100);
    enterDeepSleep();
  }
  else
  {
    // 无连接，直接深度睡眠
    Serial.printf("无连接，进入深度睡眠 %d 秒", DEEPSLEEP_TIME / 1000);
    enterDeepSleep();
  }
}

void loop()
{
  // 所有逻辑均在 setup 中完成，loop 为空（深度睡眠后设备重启，不会执行到此处）
  delay(1000);
}