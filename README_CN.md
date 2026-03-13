# MimiClaw ESP32-C3 极速上手手册 (大陆优化版)

> [!NOTE]
> 本项目源于原本的 [mimiclaw](https://github.com/memovai/mimiclaw) 项目。

MimiClaw 是一个运行在 ESP32 上的个人 AI 助手。本项目已经针对 **ESP32-C3** (无 PSRAM 版本) 进行了极致的内存优化，并针对国内网络环境进行了专项适配。

---

## 🚀 硬件要求
- **芯片**: ESP32-C3 (单核 RISC-V)
- **内存**: 内部 400KB SRAM (无需外部 PSRAM)
- **闪存**: 至少 4MB Flash
- **开发板**: 常见的 ESP32-C3 开发板均可

## 🛠️ 环境准备
1. **安装 ESP-IDF**:
   建议使用 ESP-IDF **v5.4.1** 或更高版本。
   ```bash
   # MacOS 示例
   ./install.sh esp32c3
   source ./export.sh
   ```

2. **获取代码**:
   确保你使用的是已经合并了 C3 适配补丁的分支。

## 📦 编译与刷机
本项目使用特定的 C3 配置文件和分区表。

1. **设置目标**:
   ```bash
   idf.py set-target esp32c3
   ```

2. **编译固件**:
   ```bash
   idf.py build
   ```

3. **烧录到设备**:
   ```bash
   # 这里的 /dev/cu.usbmodem* 替换为你的实际串口地址
   idf.py -p /dev/cu.usbmodem* flash monitor
   ```

4. **合并固件 (可选)**:
   如果你需要发布完整的 Bin 文件，可以使用：
   ```bash
   python -m esptool --chip esp32c3 merge_bin -o mimiclaw_c3_merged.bin --flash_mode dio --flash_size 4MB 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xf000 build/ota_data_initial.bin 0x20000 build/mimiclaw.bin 0x320000 build/spiffs.bin
   ```

---

## ⚙️ 核心优化说明 (针对 C3)
*   **内存管理**: 开启了 MbedTLS 动态 Buffer，并将 TLS 输出块限制在 2KB，防止在碎片化的堆内存中分配失败。
*   **网络适配**: 
    *   **时间同步**: 自动通过 `baidu.com` 同步时间，无需翻墙，且使用 HTTP HEAD 请求（零 TLS 内存开销）。
    *   **证书校验**: 默认开启证书跳过模式，避免加载庞大的根证书包（节省 ~40KB 内存）。
*   **存储优化**: 采用 DJB2 Hashing 缩短会话文件名，完美解决 SPIFFS 31 字符的文件名长度限制。
*   **思考过滤**: 自动识别并剥离 LLM 返回的 `<think>` 标签内容，提供纯净的对话体验。

---

## ⌨️ 首次使用配置
刷机完成后，通过串口助手 (波特率 115200) 输入以下命令进行配置：

1. **连接 WiFi**:
   ```text
   set_wifi <SSID> <PASSWORD>
   ```

2. **配置大模型 (以 MiniMax 为例)**:
   ```text
   set_model_provider openai
   set_base_url https://api.minimax.chat/v1/chat/completions
   set_api_key <你的API_KEY>
   set_model MiniMax-M2.5-highspeed
   ```

3. **配置飞书机器人 (Channel)**:
   ```text
   set_feishu_creds <APP_ID> <APP_SECRET>
   ```

---

## 🛠️ 常见问题 (Troubleshooting)
*   **ESP_ERR_NO_MEM**: 如果遇到内存报错，请检查 `mimi_config.h` 中的 `MIMI_AGENT_MAX_HISTORY` (建议设为 5) 和 `MIMI_LLM_STREAM_BUF_SIZE`。
*   **获取时间失败**: 确保设备能正常访问 `baidu.com`。
*   **无法记住上下文**: 检查串口是否有 SPIFFS 挂载成功的日志，确保 `spiffs.bin` 已正确烧录。

---

## 📖 CLI 全命令手册 (控制台命令大全)

在串口终端输入 `help` 可以查看简易说明。以下是所有可用命令的详细分类：

### 1. 网络与基础配置
*   **`set_wifi <SSID> <PASSWORD>`**: 保存 WiFi 账号密码。
*   **`wifi_status`**: 查看当前 WiFi 连接状态和 IP 地址。
*   **`wifi_scan`**: 扫描附近的 WiFi 热点。
*   **`restart`**: 重启设备。

### 2. 大语言模型 (LLM) 配置
*   **`set_model_provider <provider>`**: 设置模型供应商 (支持 `openai` 或 `anthropic`)。
*   **`set_api_key <KEY>`**: 设置 LLM API Key。
*   **`set_base_url <URL>`**: 设置 API 代理地址或自定义 Base URL。
*   **`set_model <MODEL_NAME>`**: 设置模型名称 (如 `gpt-4o` 或 `MiniMax-M2.5-highspeed`)。

### 3. 聊天频道配置
*   **`set_feishu_creds <APP_ID> <APP_SECRET>`**: 配置飞书机器人凭据。
*   **`feishu_send <RECEIVE_ID> <TEXT>`**: 手动向飞书发送一条测试消息。
*   **`set_tg_token <TOKEN>`**: 配置 Telegram Bot Token (国内连不上则无效)。

### 4. 搜索与代理
*   **`set_search_key <KEY>`**: 设置 Brave Search API Key。
*   **`set_tavily_key <KEY>`**: 设置 Tavily Search API Key。
*   **`web_search <QUERY>`**: 手动执行一次 Web 搜索工具测试。
*   **`set_proxy <HOST> <PORT> [TYPE]`**: 设置网络代理 (TYPE 支持 `http` 或 `socks5`)。
*   **`clear_proxy`**: 清除代理配置。

### 5. 存储与会话管理
*   **`config_show`**: 查看当前所有配置（NVS覆盖及编译默认值）。
*   **`config_reset`**: 清除所有 NVS 持久化配置，恢复出厂默认（需重启）。
*   **`session_list`**: 列出当前 SPIFFS 中保存的所有会话。
*   **`session_clear <CHAT_ID>`**: 清除指定会话的上下文历史。
*   **`memory_read`**: 读取长期记忆文件 (`MEMORY.md`)。
*   **`memory_write <CONTENT>`**: 写入长期记忆内容。

### 6. 技能与工具
*   **`skill_list`**: 列出当前安装的所有技能。
*   **`skill_show <NAME>`**: 查看某个技能的 Markdown 定义文件。
*   **`skill_search <KEYWORD>`**: 在技能库中搜索关键字。
*   **`tool_exec <NAME> [JSON]`**: 直接执行某个工具（调试用）。

### 7. 系统调试
*   **`heap_info`**: 查看实时内存堆占用情况 (对 C3 调优至关重要)。
*   **`heartbeat_trigger`**: 手动触发一次心跳检查。
*   **`cron_start`**: 立即启动定时任务计时器。

---
祝使用愉快！MimiClaw 随时为你待命。🐶
