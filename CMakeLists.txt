# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


# 1. Start with the base defaults file
set(SDK_DEFAULTS_LIST "sdkconfig.defaults")

# 2. Add the environment-specific defaults if the variable exists
if(SDKCONFIG)
    # Extract just 'esp32pico' from 'sdkconfig.esp32pico'
    get_filename_component(SDK_NAME "${SDKCONFIG}" EXT)
    string(REPLACE "." "" SDK_NAME "${SDK_NAME}") # Removes the leading dot
    
    # Append the specific file to the list (e.g., sdkconfig.defaults.esp32pico)
    list(APPEND SDK_DEFAULTS_LIST "sdkconfig.defaults.${SDK_NAME}")
endif()

# 3. Apply the list to the ESP-IDF variable
set(SDKCONFIG_DEFAULTS "${SDK_DEFAULTS_LIST}")


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mimiclaw)

# Pre-flash a valid SPIFFS image so first boot does not need runtime formatting.
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)
