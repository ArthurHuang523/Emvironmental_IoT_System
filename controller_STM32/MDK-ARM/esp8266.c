/**
 * ESP8266 Driver for STM32 Bare Metal
 * Based on ESP_AT_LIB official library
 * Optimized for Ai-Thinker AT Firmware 1471
 */

#include "main.h"
#include "hardware.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Configuration */
#define ESP_CFG_MAX_CONNS           5
#define ESP_CFG_CONN_MAX_DATA_LEN   1024
#define ESP_USART                   huart2
#define ESP_RESET_PIN               ESP8266_RST_Pin
#define ESP_RESET_PORT              ESP8266_RST_GPIO_Port

/* Buffer configuration */
#define ESP_RX_BUFFER_SIZE          512
#define ESP_AT_BUFFER_SIZE          256

/* Timeouts */
#define ESP_TIMEOUT_RESET           3000
#define ESP_TIMEOUT_CMD             2000
#define ESP_TIMEOUT_CONN            20000

/* ESP8266 AT responses */
#define ESP_AT_CMD_OK               "OK"
#define ESP_AT_CMD_ERROR            "ERROR"
#define ESP_AT_CMD_FAIL             "FAIL"
#define ESP_AT_CMD_READY            "ready"
#define ESP_AT_CMD_WIFI_CONNECTED   "WIFI CONNECTED"
#define ESP_AT_CMD_WIFI_GOT_IP      "WIFI GOT IP"
#define ESP_AT_CMD_MQTTCONNECTED    "MQTTCONNECTED"

/* Internal buffers */
static char esp_rx_buffer[ESP_RX_BUFFER_SIZE];
static char esp_at_buffer[ESP_AT_BUFFER_SIZE];

/* Status variables */
typedef enum {
    ESP_STATE_IDLE = 0,
    ESP_STATE_BUSY,
    ESP_STATE_ERROR
} esp_state_t;

static esp_state_t esp_state = ESP_STATE_IDLE;
static WiFi_Status_t wifi_status = WIFI_DISCONNECTED;
static MQTT_Status_t mqtt_status = MQTT_DISCONNECTED;

/* Debug function */
void debug_printf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
}


void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/**
 * \brief           Send data to ESP device
 * \param[in]       data: Pointer to data to send
 * \param[in]       len: Number of bytes to send
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
static ESP8266_Status_t esp_ll_send(const void* data, size_t len)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit(&ESP_USART, (uint8_t*)data, len, 1000);
    return (status == HAL_OK) ? ESP8266_OK : ESP8266_ERROR;
}

/**
 * \brief           Hardware reset ESP device
 */
static void esp_ll_reset(void)
{
		// Pull the pin down and then pull it up for reset	
		// debug_printf("[ESP] Hardware reset...\r\n");
    HAL_GPIO_WritePin(ESP_RESET_PORT, ESP_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(ESP_RESET_PORT, ESP_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(ESP_TIMEOUT_RESET);
    
    // Clear RX buffer
    uint8_t dummy;
    while (HAL_UART_Receive(&ESP_USART, &dummy, 1, 10) == HAL_OK);
    // debug_printf("[ESP] Reset complete\r\n");
}

/**
 * \brief           Receive data with timeout
 * \param[out]      buffer: Buffer to store received data
 * \param[in]       len: Maximum number of bytes to receive
 * \param[in]       timeout: Timeout in milliseconds
 * \return          Number of bytes actually received
 */
static size_t esp_ll_receive(void* buffer, size_t len, uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    uint8_t* buf = (uint8_t*)buffer;
    size_t received = 0;
    
    while (received < len && (HAL_GetTick() - start_time) < timeout) {
        if (HAL_UART_Receive(&ESP_USART, &buf[received], 1, 50) == HAL_OK) {
            received++;
        }
    }
    
    return received;
}

/**
 * \brief           Send AT command and wait for response
 * \param[in]       cmd: Command to send (without \r\n)
 * \param[in]       resp: Expected response string (NULL for any OK response)
 * \param[in]       timeout: Timeout in milliseconds
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
static ESP8266_Status_t esp_send_cmd(const char* cmd, const char* resp, uint32_t timeout)
{
    if (esp_state != ESP_STATE_IDLE) {
        return ESP8266_ERROR;
    }
    
    esp_state = ESP_STATE_BUSY;
    
    /* Clear buffers */
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    memset(esp_at_buffer, 0, sizeof(esp_at_buffer));
    
    /* Prepare AT command */
    if (strncmp(cmd, "AT", 2) != 0) {
        snprintf(esp_at_buffer, sizeof(esp_at_buffer), "AT%s\r\n", cmd);
    } else {
        snprintf(esp_at_buffer, sizeof(esp_at_buffer), "%s\r\n", cmd);
    }
    
    // debug_printf("[TX] %s", esp_at_buffer);
    
    /* Send command */
    if (esp_ll_send(esp_at_buffer, strlen(esp_at_buffer)) != ESP8266_OK) {
        esp_state = ESP_STATE_IDLE;
        return ESP8266_ERROR;
    }
    
    /* Wait for response */
    uint32_t start_time = HAL_GetTick();
    size_t rx_len = 0;
    
    while ((HAL_GetTick() - start_time) < timeout) {
        size_t received = esp_ll_receive(&esp_rx_buffer[rx_len], 
                                       sizeof(esp_rx_buffer) - rx_len - 1, 100);
        if (received > 0) {
            rx_len += received;
            esp_rx_buffer[rx_len] = '\0';
            
            /* Check for expected response */
            if (resp && strstr(esp_rx_buffer, resp)) {
                //debug_printf("[RX] Found: %s\r\n", resp);
                //debug_printf("[RX] Buffer: %s\r\n", esp_rx_buffer);
                esp_state = ESP_STATE_IDLE;
                return ESP8266_OK;
            }
            
            /* Check for standard responses */
            if (strstr(esp_rx_buffer, ESP_AT_CMD_OK)) {
                //debug_printf("[RX] OK\r\n");
                //debug_printf("[RX] Buffer: %s\r\n", esp_rx_buffer);
                esp_state = ESP_STATE_IDLE;
                return ESP8266_OK;
            }
            
            if (strstr(esp_rx_buffer, ESP_AT_CMD_ERROR) || 
                strstr(esp_rx_buffer, ESP_AT_CMD_FAIL)) {
                //debug_printf("[RX] ERROR/FAIL\r\n");
                //debug_printf("[RX] Buffer: %s\r\n", esp_rx_buffer);
                esp_state = ESP_STATE_IDLE;
                return ESP8266_ERROR;
            }
            
            /* WiFi specific responses */
            if (strstr(esp_rx_buffer, ESP_AT_CMD_WIFI_CONNECTED) ||
                strstr(esp_rx_buffer, ESP_AT_CMD_WIFI_GOT_IP)) {
                //debug_printf("[RX] WiFi Connected\r\n");
                //debug_printf("[RX] Buffer: %s\r\n", esp_rx_buffer);
                wifi_status = WIFI_CONNECTED;
                esp_state = ESP_STATE_IDLE;
                return ESP8266_OK;
            }
            
            /* MQTT specific responses */
            if (strstr(esp_rx_buffer, ESP_AT_CMD_MQTTCONNECTED)) {
                //debug_printf("[RX] MQTT Connected\r\n");
                //debug_printf("[RX] Buffer: %s\r\n", esp_rx_buffer);
                mqtt_status = MQTT_CONNECTED;
                esp_state = ESP_STATE_IDLE;
                return ESP8266_OK;
            }
        }
        
        HAL_Delay(10);
    }   
    //debug_printf("[RX] TIMEOUT (%d bytes)\r\n", rx_len);
    if (rx_len > 0) {
        //debug_printf("[RX] Partial: %s\r\n", esp_rx_buffer);
    }   
    esp_state = ESP_STATE_IDLE;
    return ESP8266_TIMEOUT_ERROR;
}

/**
 * \brief           Initialize ESP device
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_Init(void)
{
    /* Hardware reset */
    esp_ll_reset(); 
    /* Test basic communication */
    for (int i = 0; i < 3; i++) {
        // debug_printf("[ESP] AT test %d/3...\r\n", i + 1);
        if (esp_send_cmd("AT", NULL, ESP_TIMEOUT_CMD) == ESP8266_OK) {
            // debug_printf("[ESP] Communication OK\r\n");
            break;
        }
        if (i == 2) {
            // debug_printf("[ESP] Communication FAILED\r\n");
            return ESP8266_ERROR;
        }
        HAL_Delay(1000);
    }
    
    /* Disable echo */
    esp_send_cmd("ATE0", NULL, ESP_TIMEOUT_CMD);
    
    /* Set station mode */
    if (esp_send_cmd("+CWMODE=1", NULL, ESP_TIMEOUT_CMD) != ESP8266_OK) {
        // debug_printf("[ESP] Set station mode FAILED\r\n");
        return ESP8266_ERROR;
    }
    
    /* Get version info */
    esp_send_cmd("+GMR", NULL, ESP_TIMEOUT_CMD);
    
    // debug_printf("[ESP] Initialization complete\r\n");
    return ESP8266_OK;
}

/**
 * \brief           Connect to WiFi network
 * \param[in]       ssid: SSID of network
 * \param[in]       pass: Password of network
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_ConnectWiFi(char* ssid, char* password)
{
    char cmd[128];
    
    // debug_printf("[WiFi] Connecting to %s...\r\n", ssid);
    wifi_status = WIFI_CONNECTING;
    
    /* Disconnect first */
    esp_send_cmd("+CWQAP", NULL, 3000);
    HAL_Delay(1000);
    
    /* Connect to WiFi */
    snprintf(cmd, sizeof(cmd), "+CWJAP=\"%s\",\"%s\"", ssid, password);
    
    if (esp_send_cmd(cmd, ESP_AT_CMD_WIFI_CONNECTED, ESP_TIMEOUT_CONN) == ESP8266_OK) {
        // debug_printf("[WiFi] Connected successfully\r\n");
        wifi_status = WIFI_CONNECTED;
        
        /* Get IP address */
        esp_send_cmd("+CIFSR", NULL, ESP_TIMEOUT_CMD);
        return ESP8266_OK;
    } else {
        // debug_printf("[WiFi] Connection FAILED\r\n");
        wifi_status = WIFI_DISCONNECTED;
        return ESP8266_ERROR;
    }
}

/**
 * \brief           Disconnect from WiFi network
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_DisconnectWiFi(void)
{
    wifi_status = WIFI_DISCONNECTED;
    return esp_send_cmd("+CWQAP", NULL, ESP_TIMEOUT_CMD);
}

/**
 * \brief           Get WiFi connection status
 * \return          Current WiFi status
 */
WiFi_Status_t ESP8266_GetWiFiStatus(void)
{
    if (esp_send_cmd("+CWJAP?", NULL, ESP_TIMEOUT_CMD) == ESP8266_OK) {
        if (strstr(esp_rx_buffer, "No AP") == NULL && 
            strstr(esp_rx_buffer, "+CWJAP:") != NULL) {
            wifi_status = WIFI_CONNECTED;
        } else {
            wifi_status = WIFI_DISCONNECTED;
        }
    } else {
        wifi_status = WIFI_DISCONNECTED;
    }
    
    return wifi_status;
}

/**
 * \brief           Connect to MQTT broker
 * \param[in]       host: MQTT broker host
 * \param[in]       port: MQTT broker port
 * \param[in]       client_id: MQTT client ID
 * \param[in]       username: MQTT username (can be empty)
 * \param[in]       password: MQTT password (can be empty)
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_ConnectMQTT(char* server, uint16_t port, char* client_id, char* username, char* password)
{
    char cmd[256];    
    // debug_printf("[MQTT] Connecting to %s:%d...\r\n", server, port);
    mqtt_status = MQTT_CONNECTING;
    
    /* Configure MQTT user settings */
    snprintf(cmd, sizeof(cmd), "+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
             client_id, username, password);
    
    if (esp_send_cmd(cmd, NULL, ESP_TIMEOUT_CMD) != ESP8266_OK) {
        // debug_printf("[MQTT] User config FAILED\r\n");
        mqtt_status = MQTT_DISCONNECTED;
        return ESP8266_ERROR;
    }
    
    /* Connect to MQTT broker */
    snprintf(cmd, sizeof(cmd), "+MQTTCONN=0,\"%s\",%d,1", server, port);
    
    if (esp_send_cmd(cmd, ESP_AT_CMD_MQTTCONNECTED, 15000) == ESP8266_OK) {
        // debug_printf("[MQTT] Connected successfully\r\n");
        mqtt_status = MQTT_CONNECTED;
        return ESP8266_OK;
    } else {
        debug_printf("[MQTT] Connection FAILED\r\n");
        mqtt_status = MQTT_DISCONNECTED;
        return ESP8266_ERROR;
    }
}

/**
 * \brief           Publish MQTT message
 * \param[in]       topic: MQTT topic
 * \param[in]       payload: Message payload
 * \param[in]       qos: Quality of service (0, 1, or 2)
 * \param[in]       retain: Retain flag
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_PublishMQTT(char* topic, char* message, uint8_t qos, uint8_t retain)
{
    char cmd[512];
    
    if (mqtt_status != MQTT_CONNECTED) {
        // debug_printf("[MQTT] Not connected\r\n");
        return ESP8266_ERROR;
    }
    
    snprintf(cmd, sizeof(cmd), "+MQTTPUB=0,\"%s\",\"%s\",%d,%d", topic, message, qos, retain);
    
    // debug_printf("[MQTT] Publishing to %s: %s\r\n", topic, message);
    
    if (esp_send_cmd(cmd, NULL, 5000) == ESP8266_OK) {
        // debug_printf("[MQTT] Publish SUCCESS\r\n");
        return ESP8266_OK;
    } else {
        // debug_printf("[MQTT] Publish FAILED\r\n");
        return ESP8266_ERROR;
    }
}

/**
 * \brief           Subscribe to MQTT topic
 * \param[in]       topic: MQTT topic to subscribe
 * \param[in]       qos: Quality of service
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_SubscribeMQTT(char* topic, uint8_t qos)
{
    char cmd[256];
    
    if (mqtt_status != MQTT_CONNECTED) {
        // debug_printf("[MQTT] Not connected\r\n");
        return ESP8266_ERROR;
    }
    
    snprintf(cmd, sizeof(cmd), "+MQTTSUB=0,\"%s\",%d", topic, qos);
    
    // debug_printf("[MQTT] Subscribing to: %s\r\n", topic);
    
    if (esp_send_cmd(cmd, NULL, 5000) == ESP8266_OK) {
        // debug_printf("[MQTT] Subscribe SUCCESS\r\n");
        return ESP8266_OK;
    } else {
        // debug_printf("[MQTT] Subscribe FAILED\r\n");
        return ESP8266_ERROR;
    }
}

/**
 * \brief           Get MQTT connection status
 * \return          Current MQTT status
 */
MQTT_Status_t ESP8266_GetMQTTStatus(void)
{
    return mqtt_status;
}

/**
 * \brief           Disconnect from MQTT broker
 * \return          espOK on success, member of \ref espr_t enumeration otherwise
 */
ESP8266_Status_t ESP8266_DisconnectMQTT(void)
{
    mqtt_status = MQTT_DISCONNECTED;
    return esp_send_cmd("+MQTTCLEAN=0", NULL, ESP_TIMEOUT_CMD);
}

/**
 * \brief           Get internal buffer content
 * \return          Pointer to internal buffer
 */
char* ESP8266_GetBuffer(void)
{
    return esp_rx_buffer;
}

/**
 * \brief           Clear internal buffers
 */
void ESP8266_ClearBuffer(void)
{
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    memset(esp_at_buffer, 0, sizeof(esp_at_buffer));
}
/**
 * \brief           Send AT command (public wrapper)
 * \param[in]       cmd: Command to send (without AT prefix and \r\n)
 * \param[in]       timeout: Timeout in milliseconds
 * \return          ESP8266_Status_t on success, member of enumeration otherwise
 */
ESP8266_Status_t ESP8266_SendCommand(const char* cmd, uint32_t timeout)
{
    return esp_send_cmd(cmd, NULL, timeout);
}

/**
 * \brief           Send AT command with expected response
 * \param[in]       cmd: Command to send (without AT prefix and \r\n)
 * \param[in]       expected_resp: Expected response string
 * \param[in]       timeout: Timeout in milliseconds
 * \return          ESP8266_Status_t on success, member of enumeration otherwise
 */
ESP8266_Status_t ESP8266_SendCommandWithResponse(const char* cmd, const char* expected_resp, uint32_t timeout)
{
    return esp_send_cmd(cmd, expected_resp, timeout);
}
