/*
 * Copyright (c) 2014, Sascha Schade
 * Copyright (c) 2014-2018, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/board.hpp>

#include <modm/processing.hpp>
#include <modm/driver/color/tcs3472.hpp>
#include <modm/driver/color/veml6040.hpp>
#include <modm/driver/color/veml6070.hpp>
#include <modm/debug.hpp>

#include <modm/architecture/interface/gpio.hpp>
#include <cli.hpp>

using namespace modm::literals;

// Set the log level
#undef	MODM_LOG_LEVEL
#define	MODM_LOG_LEVEL  modm::log::DEBUG

#define stream	modm::log::info
/**
 * Example to demonstrate a MODM driver for colour sensor TCS3472
 *
 * This example uses I2cMaster2 of STM32F410
 *
 * SDA PB9
 * SCL PB8
 *
 * GND and +3V3 are connected to the colour sensor.
 *
 */
using namespace Board;

typedef I2cMaster1 MyI2cMaster;
// typedef I2cMaster2 MyI2cMaster;
// typedef BitBangI2cMaster<GpioB8, GpioB9> MyI2cMaster;
// using MyI2cMaster = BitBangI2cMaster<Board::D15, Board::D14>;

Cli		cli(modm::log::info);
Tcs		tcsCmd(cli);
V6040	v6040Cmd(cli);
V6070	v6070Cmd(cli);
// ----------------------------------------------------------------------------

#define PT_CASE(x)					\
	this->x			= __LINE__;		\
	this->ptState	= __LINE__;		\
	modm_fallthrough;				\
	case __LINE__:					\

#define PT_CASE_SET(x) ({			\
	this->ptState	= x;			\
	break;							\
})
// ----------------------------------------------------------------------------

class ThreadOne : public modm::pt::Protothread
{
public:
	ThreadOne(Cli& cli) : _cli(cli), colorSensor()
	{
	}

	bool
	update(Cli::Cmd& ctl) {
		PT_BEGIN();

		stream << "Ping the device from ThreadOne" << modm::endl;

		// ping the device until it responds
		while (true) {
			// we wait until the task started
			if (PT_CALL(colorSensor.ping())) {
			 	break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}
		MODM_LOG_DEBUG << "Device responded" << modm::endl;

		while (true) {
			if (PT_CALL(colorSensor.initialize())) {
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}
		MODM_LOG_DEBUG << "Device initialized" << modm::endl;

		PT_CASE(PT_ONE_CONFIG);
		while (true) {
			if (PT_CALL(colorSensor.configure(
				colorSensor.gain,
				static_cast<uint8_t>(colorSensor.integrationTime),
				static_cast<uint8_t>(colorSensor.waitTime)))){
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}

		MODM_LOG_DEBUG << "Device configured\n" << modm::endl;
		MODM_LOG_DEBUG << "Sensors data:" << modm::endl;

		while (true) {
			if (ctl == Cli::Cmd::Control) {
				stream << "Ctrl+C" << modm::endl;
				ctl = Cli::Cmd::None;
				break;
			}

			if (PT_CALL(colorSensor.refreshAllColors())) {
				auto colors = colorSensor.getOldColors();
				stream << "TCS34725" << modm::endl;
				stream.printf("RGBW Hue: %5d %5d %5d %5d", colors.red, colors.green, colors.blue, colors.white);
				modm::color::HsvT<modm::tcs3472::UnderlyingType> hsv;
				colors.toHsv(&hsv);
				stream.printf("  %5d\n", hsv.hue);
				//MODM_LOG_DEBUG << modm::flush;
			}
			this->timeout.restart(500);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}

		PT_YIELD();

		// ¬ышли по Ctrl+C из цикла и попали в ожидание команды пользовател€:
		while (true) {
			if (ctl == Cli::Cmd::Command) {
				if ( cli.command() == sensorName || cli.command() == "all" ) {
					bool ok	= true;
					tcsCmd.getOptions();

					// Restart
					if ( tcsCmd.restart | tcsCmd.init |  tcsCmd.ping ) {
						ctl = Cli::Cmd::None;
						PT_RESTART();
					}
					// wlong Bit set
					if ( tcsCmd.wlong ) {
						stream << "'wlong' option is not supported in this version" << modm::endl;
					}
					// set gain
					if ( !tcsCmd.sagain.empty() ) {
							   if ( tcsCmd.sagain == "X1" ) {
							colorSensor.gain	= modm::tcs3472::Gain::X1;
						} else if ( tcsCmd.sagain == "X4" ) {
							colorSensor.gain	=  modm::tcs3472::Gain::X4;
						} else if ( tcsCmd.sagain == "X16" ) {
							colorSensor.gain	=  modm::tcs3472::Gain::X16;
						} else if ( tcsCmd.sagain == "X60" ) {
							colorSensor.gain	= modm::tcs3472::Gain::X60;
						} else {
							stream << "Invalid value of option 'again'" << modm::endl;
							ok	= false;
						}
					}
					// set atime
					if ( !tcsCmd.satime.empty() ) {
							   if ( tcsCmd.satime == "2ms" ) {
							colorSensor.integrationTime	= modm::tcs3472::IntegrationTime::MSEC_2;
						} else if ( tcsCmd.satime == "24ms" ) {
							colorSensor.integrationTime	= modm::tcs3472::IntegrationTime::MSEC_24;
						} else if ( tcsCmd.satime == "101ms" ) {
							colorSensor.integrationTime	= modm::tcs3472::IntegrationTime::MSEC_101;
						} else if ( tcsCmd.satime == "154ms" ) {
							colorSensor.integrationTime	= modm::tcs3472::IntegrationTime::MSEC_154;
						} else if ( tcsCmd.satime == "700ms" ) {
							colorSensor.integrationTime	= modm::tcs3472::IntegrationTime::MSEC_700;
						} else {
							stream << "Invalid value of option 'atime'" << modm::endl;
							ok	= false;
						}
					}
					// set wtime
					if ( !tcsCmd.swtime.empty() ) {
							   if ( tcsCmd.swtime == "2ms" ) {
							colorSensor.waitTime	= modm::tcs3472::WaitTime::MSEC_2;
						} else if ( tcsCmd.swtime == "204ms" ) {
							colorSensor.waitTime	= modm::tcs3472::WaitTime::MSEC_204;
						} else if ( tcsCmd.swtime == "614ms" ) {
							colorSensor.waitTime	= modm::tcs3472::WaitTime::MSEC_614;
						} else {
							stream << "Invalid value of option 'wtime'" << modm::endl;
							ok	= false;
						}
					}

					if ( ok ) {
						PT_CASE_SET(PT_ONE_CONFIG);
					}
				}
			}
			//  оманда была обработана
			if ( ctl != Cli::Cmd::None )	ctl = Cli::Cmd::None;
			return true;
		}

		PT_END();
	}

private:
	Cli&						_cli;
	modm::ShortTimeout 			timeout;
	modm::Tcs3472<MyI2cMaster>	colorSensor;
	const char*					sensorName		= "tcs";
	const char*					sensorsName		= "all";

	int							PT_ONE_CONFIG	= 0;
};

ThreadOne one(cli);
// ----------------------------------------------------------------------------

class ThreadTwo : public modm::pt::Protothread
{
public:
	ThreadTwo(Cli& cli) : _cli(cli), colorSensor()
	{
	}

	bool
	update(Cli::Cmd& ctl) {
		PT_BEGIN();

		stream << "Ping the device from ThreadTwo" << modm::endl;

		// ping the device until it responds
		while (true) {
			// we wait until the task started
			if (PT_CALL(colorSensor.ping())) {
			 	break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}
		MODM_LOG_DEBUG << "Device responded" << modm::endl;

		while (true) {
			if (PT_CALL(colorSensor.initialize())) {
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}
		MODM_LOG_DEBUG << "Device initialized" << modm::endl;

		PT_CASE(PT_TWO_CONFIG);
		while (true) {
			if (PT_CALL(colorSensor.configure(static_cast<uint8_t>(colorSensor.integrationTime)))){
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}

		MODM_LOG_DEBUG << "Device configured\n" << modm::endl;
		MODM_LOG_DEBUG << "Sensors data:" << modm::endl;

		while (true) {
			if (ctl == Cli::Cmd::Control) {
				stream << "Ctrl+C" << modm::endl;
				ctl = Cli::Cmd::None;
				break;
			}

			if (PT_CALL(colorSensor.refreshAllColors())) {
				auto colors = colorSensor.getOldColors();
				stream << "VEML6040" << modm::endl;
				stream.printf("RGBW Hue: %5d %5d %5d %5d", colors.red, colors.green, colors.blue, colors.white);
				modm::color::HsvT<modm::tcs3472::UnderlyingType> hsv;
				colors.toHsv(&hsv);
				stream.printf("  %5d\n", hsv.hue);
			}
			this->timeout.restart(500);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}

		PT_YIELD();

		// ¬ышли по Ctrl+C из цикла и попали в ожидание команды пользовател€:
		while (true) {
			if (ctl == Cli::Cmd::Command) {
				if ( cli.command() == sensorName || cli.command() == "all" ) {
					bool ok	= true;
					v6040Cmd.getOptions();

					// Restart
					if ( v6040Cmd.restart | v6040Cmd.init |  v6040Cmd.ping ) {
						ctl = Cli::Cmd::None;
						PT_RESTART();
					}
					// set atime
					if ( !v6040Cmd.satime.empty() ) {
							   if ( v6040Cmd.satime == "1280ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_1280;
						} else if ( v6040Cmd.satime == "640ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_640;
						} else if ( v6040Cmd.satime == "320ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_320;
						} else if ( v6040Cmd.satime == "160ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_160;
						} else if ( v6040Cmd.satime == "80ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_80;
						} else if ( v6040Cmd.satime == "40ms" ) {
							colorSensor.integrationTime	= modm::veml6040::IntegrationTime::MSEC_40;
						} else {
							stream << "Invalid value of option 'atime'" << modm::endl;
							ok	= false;
						}
					}

					if ( ok ) {
						PT_CASE_SET(PT_TWO_CONFIG);
					}
				}
			}
			// —казать, что команда была обработана
			if ( ctl != Cli::Cmd::None )	ctl = Cli::Cmd::None;
			return true;
		}

		PT_END();
	}

private:
	Cli&						_cli;
	modm::ShortTimeout 			timeout;
	modm::Veml6040<MyI2cMaster>	colorSensor;
	const char*					sensorName		= "v6040";
	const char*					sensorsName		= "all";

	int 						PT_TWO_CONFIG	= 0;
};

ThreadTwo two(cli);
// ----------------------------------------------------------------------------

class ThreadThree : public modm::pt::Protothread
{
public:
	ThreadThree(Cli& cli) : _cli(cli), colorSensor()
	{
	}

	bool
	update(Cli::Cmd& ctl) {
		PT_BEGIN();

		stream << "Ping the device from ThreadThree" << modm::endl;

		// ping the device until it responds
		while (true) {
			// otherwise, try again in 150ms
			this->timeout.restart(150);
			PT_WAIT_UNTIL(this->timeout.isExpired());

			// we wait until the task started
			if (PT_CALL(colorSensor.ping())) {
			 	break;
			}
		}
		MODM_LOG_DEBUG << "Device responded" << modm::endl;

		while (true) {
			if (PT_CALL(colorSensor.initialize())) {
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}
		MODM_LOG_DEBUG << "Device initialized" << modm::endl;

		PT_CASE(PT_THREE_CONFIG);
		while (true) {
			if (PT_CALL(colorSensor.configure(static_cast<uint8_t>(colorSensor.integrationTime)))) {
				break;
			}
			// otherwise, try again in 100ms
			this->timeout.restart(100);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}													/*

		while (true) {
			// otherwise, try again in 140ms
			this->timeout.restart(140);
			PT_WAIT_UNTIL(this->timeout.isExpired());

			if (PT_CALL(colorSensor.sleep(false))) {
				break;
			}
		}													*/

		MODM_LOG_DEBUG << "Device configured\n" << modm::endl;
		MODM_LOG_DEBUG << "Sensors data:" << modm::endl;

		while (true) {
			if (ctl == Cli::Cmd::Control) {
				stream << "Ctrl+C" << modm::endl;
				ctl = Cli::Cmd::None;
				break;
			}

			if (PT_CALL(colorSensor.refreshAllColors())) {
				auto colors = colorSensor.getOldColors();
				stream << "VEML6070" << modm::endl;
				stream.printf("Uv: %5d\n", colors.uv);
			}

			// Depends on RSET = 270K, note actual time is shorter
			// than 62.5ms for RSET = 300K in datasheet table (63 ~ 62.5ms)
			//this->timeout.restart(colorSensor.timeForNext() * 63);
			this->timeout.restart(500);
			PT_WAIT_UNTIL(this->timeout.isExpired());
		}

		PT_YIELD();

		// ¬ышли по Ctrl+C из цикла и попали в ожидание команды пользовател€:
		while (true) {
			if (ctl == Cli::Cmd::Command) {
				if ( cli.command() == sensorName || cli.command() == "all" ) {
					bool ok	= true;
					v6070Cmd.getOptions();

					// Restart
					if ( v6070Cmd.restart | v6070Cmd.init |  v6070Cmd.ping ) {
						ctl = Cli::Cmd::None;
						PT_RESTART();
					}
					// set atime
					if ( !v6070Cmd.satime.empty() ) {
							   if ( v6070Cmd.satime == "500ms" ) {
							colorSensor.integrationTime	= modm::veml6070::IntegrationTime::MSEC_500;
						} else if ( v6070Cmd.satime == "250ms" ) {
							colorSensor.integrationTime	= modm::veml6070::IntegrationTime::MSEC_250;
						} else if ( v6070Cmd.satime == "125ms" ) {
							colorSensor.integrationTime	= modm::veml6070::IntegrationTime::MSEC_125;
						} else if ( v6070Cmd.satime == "62.5ms" ) {
							colorSensor.integrationTime	= modm::veml6070::IntegrationTime::MSEC_62_5;
						} else {
							stream << "Invalid value of option 'atime'" << modm::endl;
							ok	= false;
						}
					}

					if ( ok ) {
						PT_CASE_SET(PT_THREE_CONFIG);
					}
				}
			}
			// —казать, что команда была обработана
			if ( ctl != Cli::Cmd::None )	ctl = Cli::Cmd::None;
			return true;
		}

		PT_END();
	}

private:
	Cli&						_cli;
	modm::ShortTimeout 			timeout;
	modm::Veml6070<MyI2cMaster>	colorSensor;
	const char*					sensorName		= "v6070";
	const char*					sensorsName		= "all";

	int							PT_THREE_CONFIG	= 0;
};

ThreadThree three(cli);
// ----------------------------------------------------------------------------
void
usart2PostInit() {
	UsartHal2::enableInterruptVector(true, 14);
	UsartHal2::enableInterrupt(UsartHal2::Interrupt::RxNotEmpty);
	UsartHal2::setReceiverEnable(true);
}
// ----------------------------------------------------------------------------

enum PT_ATTRS {
	PT_NUMS	= 3,
	PT_1	= 0,
	PT_2	= 1,
	PT_3	= 2
};

int
main() {
	Board::initialize();               
  
    LedD13::setOutput(modm::Gpio::Low);

    usart2PostInit();

	MyI2cMaster::connect<GpioB9::Sda, GpioB8::Scl>();
	MyI2cMaster::initialize<Board::SystemClock, 100_kHz>();

	stream << "\n\nApplication has started\n\n" << modm::flush;
	stream << "Trying to work with TCS34725/VEML6040 RGB and VEML6070 UV sensors (I2C boadrate=100KHz):\n\n" << modm::flush;

	modm::ShortPeriodicTimer tmr(500);

	Cli::Cmd	ctl;
	Cli::Cmd	ctls[PT_ATTRS::PT_NUMS];
	bool		showPrompt	= false;

	cli.prompt();

	while (true) {
		// ѕроверка входного потока команд
		ctl	= cli.checkInput();
		if ( (ctl != Cli::Cmd::None) && (ctl != Cli::Cmd::Error) ) {
			for (uint8_t i=0; i<3; i++) ctls[i] = ctl;
			showPrompt	= true;
		}

		one		.update(ctls[PT_ATTRS::PT_1]);
		two		.update(ctls[PT_ATTRS::PT_2]);
		three	.update(ctls[PT_ATTRS::PT_3]);

		// ≈сли все потоки отработали свои команды -
		bool	allDone	= true;
		for (uint8_t i=0; i<3; i++)
			allDone &=  ( ctls[i] == Cli::Cmd::None )? true: false;
		// выводим приглашение
		if ( allDone && showPrompt ) {
			cli.done();
			showPrompt	= false;
		}

		if (tmr.execute()) {
			LedD13::toggle();
		}
	}

	return 0;
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------

