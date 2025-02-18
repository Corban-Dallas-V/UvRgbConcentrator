// coding: utf-8
/*
 * Copyright (c) 2017, Arjun Sarin
 * Copyright (c) 2017-2018, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

/**
 * \file	veml6040.hpp
 * \date	15 Nov 2019
 * \author	David Hebbeker, Arjun Sarin, Alex Zdr
 */

#ifndef MODM_VEML6040_HPP
#define MODM_VEML6040_HPP

#include <stdint.h>

#include <modm/ui/color.hpp>
#include <modm/architecture/interface/i2c_device.hpp>

namespace modm
{
/**
 * \brief 	Settings to configure the vishay digital color sensor family veml6040
 * \see		veml6040
 * \ingroup	modm_driver_veml6040
 *
 * Device   Address
 * veml6040  0x10 (with IR filter)
 *        3  0x10 (without IR filter)
 *        5  0x10 (with IR filter)
 *        7  0x10 (without IR filter)
 *
 */
struct veml6040
{

	//! \brief 	Integration for a fixed time
	enum class IntegrationTime : uint8_t
	{
		MSEC_1280			= 0x50,	//!< integrate over 1280 ms
		MSEC_640			= 0x40,	//!< integrate over 640 ms
		MSEC_320			= 0x30,	//!< integrate over 320 ms
		MSEC_160			= 0x20,	//!< integrate over 160 ms
		MSEC_80 			= 0x10,	//!< integrate over 80 ms
		MSEC_40   			= 0x00,	//!< integrate over 40 ms
		DEFAULT   			= 0x00	//!< default value on chip reset
	};
	//! @}


	//! \brief 	Register addresses
	enum class RegisterAddress : uint8_t
	{
		ENABLE				= 0x00,	//!< Primarily to power up the device and timing
		// Reserved registers
		//RESERVED_1		= 0x01,	//!<
		//RESERVED_2		= 0x02,	//!<
		//RESERVED_3		= 0x03,	//!<
		//RESERVED_4		= 0x4,	//!<
		//RESERVED_5		= 0x5,	//!<
		//RESERVED_6		= 0x06,	//!<
		//RESERVED_7		= 0x07,	//!<
		// Data registers
		RDATALOW			= 0x08,	//!< Low byte of ADC red channel
		RDATAHIGH			= 0x08,	//!< High byte of ADC red channel
		GDATALOW			= 0x09,	//!< Low byte of ADC green channel
		GDATAHIGH			= 0x09,	//!< High byte of ADC green channel
		BDATALOW			= 0x0A,	//!< Low byte of ADC blue channel
		BDATAHIGH			= 0x0A,	//!< High byte of ADC blue channel
		CDATALOW			= 0x0B,	//!< Low byte of ADC clear channel
		CDATAHIGH			= 0x0B,	//!< High byte of ADC clear channel
	};

	typedef uint16_t	UnderlyingType;		//!< datatype of color values
	typedef color::RgbwT<UnderlyingType> Rgbw;

};

/**
 * \brief	Veml6040X Digital Color Sensors
 *
 *
 * \tparam	I2CMaster	I2C interface which needs an \em initialized
 * 						modm::i2c::Master
 * \see		veml6040
 * \author	David Hebbeker, Arjun Sarin, Alex Zdr
 * \ingroup	modm_driver_veml6040
 */
template < typename I2cMaster >
class Veml6040 : public veml6040, public modm::I2cDevice< I2cMaster, 2 >
{
public:
	Veml6040(uint8_t address = 0x10);

	//! \brief 	Power up sensor and start conversions
	// Blocking
	bool inline
	initializeBlocking()
	{
		return RF_CALL_BLOCKING(initialize());
	}

	/**
	 * @name Return already sampled color
	 * @{
	 */
	inline static Veml6040::Rgbw
	getOldColors()
	{
		return color;
	};

	//!@}

	/**
	 * @name Sample and return fresh color values
	 * @{
	 */
	inline static Veml6040::Rgbw
	getNewColors()
	{
		refreshAllColors();
		return getOldColors();
	};

	//!@}

	//! \brief	Read current samples of ADC conversions for all channels.
	// Non-blocking
	modm::ResumableResult<bool>
	refreshAllColors();

	// MARK: - TASKS
	modm::ResumableResult<bool>
	initialize()
	{
				writeRegister(RegisterAddress::ENABLE, 0x21);	// control to power off
		return	writeRegister(RegisterAddress::ENABLE, 0x20);	// control to power up and start conversion
	};

	modm::ResumableResult<bool>
	configure(const uint8_t int_time    = IntegrationTime::DEFAULT);

private:
	//! \brief Sets the integration time for the ADCs.
	modm::ResumableResult<bool>
	setIntegrationTime(const uint8_t int_time = 0)
	{
		return writeRegister(RegisterAddress::ENABLE, static_cast<uint8_t>(int_time));
	}

private:
	uint8_t commandBuffer[4];
	bool success;

private:
	//! \brief	Read value of specific register.
	modm::ResumableResult<bool>
	readRegisters(
			const RegisterAddress address,
			uint8_t * const values,
			const uint8_t count = 1);

	modm::ResumableResult<bool>
	writeRegister(
			const RegisterAddress address,
			const uint8_t value);

private:
	class uint16_t_LOW_HIGH
	{
	private:
		uint8_t low;
		uint8_t high;
	public:
		uint16_t
		get() const
		{
			uint16_t
			value  = low;
			value |= high << 8;
			return value;
		}
		inline uint8_t getLSB()	const { return low; }
		inline uint8_t getMSB()	const { return high; }
	} modm_packed;

	static union Data
	{
		uint8_t dataBytes[2*4];
		struct
		{
			uint16_t_LOW_HIGH red;
			uint16_t_LOW_HIGH green;
			uint16_t_LOW_HIGH blue;
            uint16_t_LOW_HIGH clear;
		} modm_packed;
	} data;

	static Rgbw	color;

public:
	IntegrationTime	integrationTime;
};
}

#include "veml6040_impl.hpp"

#endif // MODM_veml6040_HPP
