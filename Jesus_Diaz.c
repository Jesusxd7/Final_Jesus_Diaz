//Funcion que configura el bus SPI segun el modo
void spi_bus_init(uint32_t mode)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_16BIT;

    switch (mode)
    {
        case 0:
            hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
            break;

        case 1:
            hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
            break;

        case 2:
            hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
            break;

        case 3:
            hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
            hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
            break;

        default:
            hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
            hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
            break;
    }

    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;

    HAL_SPI_Init(&hspi1);
}

//Funcion para permitir escribit uun valor de un registro deseado
void mcp4132_write_register(uint8_t reg, uint16_t value)
{
    uint8_t tx_data[2];


    tx_data[0] = ((reg & 0x0F) << 4) | ((value >> 8) & 0x03);


    tx_data[1] = value & 0xFF;

    HAL_GPIO_WritePin(MCP4132_CS_GPIO_Port, MCP4132_CS_Pin, GPIO_PIN_RESET);

    HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(MCP4132_CS_GPIO_Port, MCP4132_CS_Pin, GPIO_PIN_SET);
}

//Funcion para leer el valor de un registro deseado
void mcp4132_read_register(uint8_t reg)
{
    uint8_t tx_data[2];
    uint8_t rx_data[2];
    uint16_t value;

    tx_data[0] = ((reg & 0x0F) << 4) | 0x0C;

    tx_data[1] = 0x00;

    HAL_GPIO_WritePin(MCP4132_CS_GPIO_Port, MCP4132_CS_Pin, GPIO_PIN_RESET);

    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 2, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(MCP4132_CS_GPIO_Port, MCP4132_CS_Pin, GPIO_PIN_SET);

    value = ((rx_data[0] & 0x03) << 8) | rx_data[1];

    return value;
}

//Funcion que permite escribir directamente el valor de N

HAL_StatusTypeDef mcp4132_set_wiper(uint8_t n)
{
    if (n > MCP4132_WIPER_MAX)
    {
        return HAL_ERROR;
    }

    mcp4132_write_register(MCP4132_WIPER0_REG, n);

    return HAL_OK;
}

//Funcoon para que la resistencia del filtro opere a la frecuencia de corte deseada
HAL_StatusTypeDef mcp4132_set_cutoff_frequency(float fc_hz)
{
    float r_target;
    float n_float;
    uint8_t n;

    if (fc_hz <= 0.0f)
    {
        return HAL_ERROR;
    }

    r_target = 1.0f / (2.0f * 3.14159265f * fc_hz * FILTER_CAP_FARADS);

    n_float = ((r_target - MCP4132_RW_OHMS) * MCP4132_STEPS) / MCP4132_RAB_OHMS;

    if (n_float < 0.0f || n_float > 128.0f)
    {
        return HAL_ERROR;
    }

    n = (uint8_t)(n_float + 0.5f);

    return mcp4132_set_wiper(n);
}
//Profe la variables usted dijo que las tomaramos como si ya estuvieran definidas, entonces pues no defini pero si las puse como si si estuvieran
//MAIN

//Si el voltaje es mayor a 1.4 el n es igual a 95, si el voltaje es menos que 0.9 el n es igual a 42
void app_main(void)
{
    uint32_t adc_valor; 
    float voltaje;   

    HAL_ADC_Init(&hadc1);
    HAL_UART_Init(&huart2);

    HAL_TIM_Base_Start_IT(&htim2);

    while (1)
    {
        if (flag_muestreo)
        {
            flag_muestreo = 0;

            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
            adc_valor = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);

            voltaje = (adc_valor * 3.3f) / 4095.0f;

            if (voltaje >= 1.4f)
            {
                mcp4132_set_wiper(95);
            }
            else if (voltaje < 0.9f)
            {
                mcp4132_set_wiper(42);
            }
        }
    }
}
