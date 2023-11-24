"""Serial ports data logger."""
import signal

import config
from config import logger
from datalog import InfluxDataLogger, RotatingFileDataLogger
from serial_reader import SerialPortManager


def main():
    """Main application entry point."""

    # create the data logger, this will also create the influxdb client
    # InfluxDataLogger will exit the app if the token is not provided
    influx_data_logger = InfluxDataLogger()

    # create the file datalogger to write the received data as CSV files
    # with daily rotation.
    file_data_logger = RotatingFileDataLogger()

    # create the serial port manager, that will be responsible for
    # monitoring available serial ports, starting and stopping the
    # threads for each port, and reading data from the ports
    port_manager = SerialPortManager()
    port_manager.add_data_handler(influx_data_logger)
    port_manager.add_data_handler(file_data_logger)
    port_manager.start()

    # setup signal handler to stop the port manager on SIGINT
    def signal_handler(sig, frame):
        logger.info("Caught signal, stopping serial port manager...")
        port_manager.stop()
        logger.info("Stopped serial port manager, app shutdown.")

    # handle the SIGINT and SIGTERM signals
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)

    # Notify systemd that the service is ready
    # Do this only if running as a systemd service
    config.notify_systemd(config.NOTIFY_READY)


if __name__ == '__main__':
    main()
