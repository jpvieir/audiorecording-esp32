#include "I2SSampler.h"
#include "driver/i2s.h"

I2SSampler::I2SSampler(i2s_port_t i2sPort, const i2s_config_t &i2s_config) : m_i2sPort(i2sPort), m_i2s_config(i2s_config)
{
}

void I2SSampler::start()
{
    i2s_driver_install(m_i2sPort, &m_i2s_config, 0, NULL);
    configureI2S();
}

void I2SSampler::stop()
{
    unConfigureI2S();
    i2s_driver_uninstall(m_i2sPort);
}
