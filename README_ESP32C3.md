# MimiClaw ESP32-C3 Quick Start Guide (Finalized Port)

MimiClaw is a person AI assistant running on ESP32. This port is specifically tuned for the **ESP32-C3** (No PSRAM version) with extreme memory optimizations and stable connectivity for Mainland China.

---

## 🚀 Hardware Requirements
- **Chip**: ESP32-C3 (Single-core RISC-V)
- **Memory**: Internal 400KB SRAM (No external PSRAM required)
- **Flash**: 4MB Flash (or larger)
- **Board**: Any standard ESP32-C3 development board (e.g., LuatOS, Seeed Studio, NodeMCU)

## 🛠️ Environment Setup
1. **ESP-IDF Installation**:
   Recommended version: **ESP-IDF v5.4.1** or higher.
   ```bash
   # MacOS Example
   ./install.sh esp32c3
   source ./export.sh
   ```

## 📦 Build & Flash
The project uses specialized config files for the C3.

1. **Set Target**:
   ```bash
   idf.py set-target esp32c3
   ```

2. **Build**:
   ```bash
   idf.py build
   ```

3. **Flash**:
   ```bash
   # Replace /dev/cu.usbmodem* with your port
   idf.py -p /dev/cu.usbmodem* flash monitor
   ```

4. **Merge Binaries (Optional)**:
   To create a single flashable bin:
   ```bash
   python -m esptool --chip esp32c3 merge_bin -o mimiclaw_c3_merged.bin --flash_mode dio --flash_size 4MB 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xf000 build/ota_data_initial.bin 0x20000 build/mimiclaw.bin 0x320000 build/spiffs.bin
   ```

---

## ⚙️ Key C3 Optimizations
*   **Memory Management**: Enabled MbedTLS dynamic buffers and capped TLS output chunks to 2KB to prevent OOM errors on fragmented heap.
*   **Mainland China Connectivity**: 
    *   **Time Sync**: Targets `baidu.com` via plain HTTP HEAD request (zero TLS overhead).
    *   **Insecure TLS**: Bypasses full certificate chain validation by default to save ~40KB RAM.
*   **Storage**: Implemented DJB2 hashing for chat sessions to bypass SPIFFS 31-character filename limit.
*   **Response Filtering**: Automatically strips `<think>` tokens from LLM output for a cleaner UI.

---

## ⌨️ First-Time Configuration
After flashing, use a serial terminal (115200 baud) and enter these commands:

1. **Connect WiFi**:
   ```text
   set_wifi <SSID> <PASSWORD>
   ```

2. **Configure LLM (OpenAI/MiniMax Example)**:
   ```text
   set_model_provider openai
   set_base_url https://api.minimax.chat/v1/chat/completions
   set_api_key <YOUR_API_KEY>
   set_model MiniMax-M2.5-highspeed
   ```

3. **Configure Channels**:
   ```text
   set_feishu_creds <APP_ID> <APP_SECRET>
   ```

---

## 📖 Full CLI Command Reference

Type `help` in the serial terminal for a brief overview. Here is the categorized list:

### 1. Network & System
*   **`set_wifi <SSID> <PASSWORD>`**: Save WiFi credentials.
*   **`wifi_status`**: Show connection status and IP.
*   **`wifi_scan`**: Scan for nearby APs.
*   **`restart`**: Soft reboot the device.

### 2. LLM Configuration
*   **`set_model_provider <provider>`**: Set provider (`openai` or `anthropic`).
*   **`set_api_key <KEY>`**: Set API key.
*   **`set_base_url <URL>`**: Set custom portal/proxy base URL.
*   **`set_model <MODEL_NAME>`**: Set model identifier.

### 3. Channels
*   **`set_feishu_creds <APP_ID> <APP_SECRET>`**: Config Feishu bot.
*   **`feishu_send <ID> <TEXT>`**: Manual Feishu message test.
*   **`set_tg_token <TOKEN>`**: Set Telegram bot token.

### 4. Search & Proxy
*   **`set_search_key <KEY>`**: Set Brave Search API key.
*   **`set_tavily_key <KEY>`**: Set Tavily API key.
*   **`web_search <QUERY>`**: Run tool-based web search test.
*   **`set_proxy <HOST> <PORT> [TYPE]`**: Set proxy (TYPE: `http` or `socks5`).
*   **`clear_proxy`**: Remove proxy config.

### 5. Memory & Session
*   **`config_show`**: Show active configuration.
*   **`config_reset`**: Wipe NVS settings (revert to factory defaults).
*   **`session_list`**: List saved chat IDs.
*   **`session_clear <ID>`**: Wipe context for a specific chat.
*   **`memory_read`**: Read `MEMORY.md`.
*   **`memory_write <CONTENT>`**: Append to `MEMORY.md`.

### 6. Skills & Tools
*   **`skill_list`**: List all markdown skills.
*   **`skill_show <NAME>`**: Print a skill's definition.
*   **`skill_search <KW>`**: Search keyword in skill files.
*   **`tool_exec <NAME> [JSON]`**: Execute a tool manually for debugging.

### 7. Debugging
*   **`heap_info`**: Show free SRAM (Critical for C3).
*   **`heartbeat_trigger`**: Force a background task check.
*   **`cron_start`**: Start the cron timer immediately.

---
Happy Hacking! MimiClaw is ready to serve. 🐱
