# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# Configurações básicas do projeto
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configurações específicas para o Raspberry Pi Pico
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Inclui o SDK do Raspberry Pi Pico
include(pico_sdk_import.cmake)

# Define o projeto
project(Embarcatech_Projeto_Final C CXX ASM)

# Inicializa o SDK do Raspberry Pi Pico
pico_sdk_init()

# Adiciona o executável
add_executable(Embarcatech_Projeto_Final 
    Embarcatech_Projeto_Final.c 
    inc/ssd1306.c
)

# Configurações do projeto
pico_set_program_name(Embarcatech_Projeto_Final "Embarcatech_Projeto_Final")
pico_set_program_version(Embarcatech_Projeto_Final "0.1")

# Gera o cabeçalho PIO
pico_generate_pio_header(Embarcatech_Projeto_Final ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Habilita saída serial (UART/USB)
pico_enable_stdio_uart(Embarcatech_Projeto_Final 1)
pico_enable_stdio_usb(Embarcatech_Projeto_Final 1)

# Adiciona bibliotecas padrão
target_link_libraries(Embarcatech_Projeto_Final
    pico_stdlib
    hardware_pio
    hardware_i2c
    hardware_pwm
    hardware_adc
)

# Adiciona diretórios de inclusão
target_include_directories(Embarcatech_Projeto_Final PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Gera arquivos adicionais (UF2, HEX, etc.)
pico_add_extra_outputs(Embarcatech_Projeto_Final)