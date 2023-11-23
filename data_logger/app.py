"""Serial ports data logger."""
import json
import signal
import sys
import threading
import queue
from time import time_ns, sleep

import serial
from influxdb_client.client.write_api import SYNCHRONOUS
from influxdb_client import InfluxDBClient, Point
from serial.tools import list_ports
from systemd.daemon import notify

import config
from config import logger

NOTIFY_READY = "READY=1"
NOTIFY_WATCHDOG = "WATCHDOG=1"


class SerialPortThread(threading.Thread):
    """Thread for reading data from a serial port."""

    def __init__(
            self,
            port_name: str,
            stop_event: threading.Event,
            reading_queue: queue.Queue,
            max_reconnect_attempts: int = config.RECONNECT_ATTEMPTS,
            baud_rate: int = config.BAUD_RATE,
            timeout: int = config.PORT_TIMEOUT,
            reconnect_delay: int = config.PORT_RECONNECT_DELAY,
    ):
        super().__init__()
        self.port_name = port_name
        self.stop_event = stop_event
        self.reading_queue = reading_queue
        self.ser = None
        self.max_reconnect_attempts = max_reconnect_attempts
        self.baud_rate = baud_rate
        self.timeout = timeout
        self.reconnect_delay = reconnect_delay

    def run(self):
        """Main thread entry point."""

        logger.info(f"Starting serial port thread for {self.port_name}...")

        # main loop, execute until stop event is set
        while not self.stop_event.is_set():
            # connect to the serial port
            connected = self.keep_connection()
            if not connected:
                logger.error(f"Failed to connect to {self.port_name}")
                self.stop_event.set()
                break
            try:
                line = self.ser.readline().decode('utf-8').strip()
                if line:
                    self.reading_queue.put(line)
            except Exception as e:
                logger.debug(f"Error reading from {self.port_name}: {str(e)}")
                continue
        logger.info(f"Stopping serial port thread for {self.port_name}...")

    def stop(self):
        """Stop the thread."""
        if self.stop_event.is_set():
            # the thread is already stopped
            return

        logger.info(f"Stopping serial port thread for {self.port_name}...")
        self.stop_event.set()
        self.join()
        logger.info(f"Stopped serial port thread for {self.port_name}")
        if self.ser:
            logger.info(f"Closing serial port {self.port_name}...")
            self.ser.close()
            logger.info(f"Closed serial port {self.port_name}")

    def keep_connection(self):
        """Check if the connection is still alive and reconnect if needed.

        Make multiple attempts to reconnect before giving up.
        """
        attempts = 0
        while True:
            # the thread is exiting, don't reconnect
            if self.stop_event.is_set():
                logger.info(f"Stop event set for {self.port_name}, thread is exiting, not reconnecting.")
                return False
            # try to connect to the port if not connected
            if self.ser is None or not self.ser.is_open:
                logger.info(f"Attempting to connect to {self.port_name}...")
                try:
                    if port := self.connect_port(self.port_name):
                        self.ser = port
                        logger.info(f"Connected to {self.port_name}")
                        return True
                    else:
                        logger.error(f"Failed to connect to {self.port_name}")
                        attempts += 1
                        sleep(self.reconnect_delay)
                except Exception as e:
                    logger.exception(f"Failed to connect to {self.port_name}: {str(e)}")
                    attempts += 1
                    if attempts > self.max_reconnect_attempts:
                        logger.error(f"Failed to connect to {self.port_name} after {attempts} attempts")
                        return False
                    sleep(self.reconnect_delay)
                    continue
            else:
                # the port is already connected
                return True

    def connect_port(self, port_name: str) -> serial.Serial | None:
        """Connect to the specified port and return the serial object."""
        try:
            logger.info(f"Connecting to {port_name}...")
            ser = serial.Serial(port_name, self.baud_rate, timeout=self.timeout)
            logger.info(f"Successfully connected to {port_name}")
            return ser
        except Exception as e:
            logger.exception(f"Failed to connect to {port_name}: {str(e)}")
            return None


class QueueReadingThread(threading.Thread):
    """Thread for reading data from a queue."""

    def __init__(self, queue: queue.Queue, callback: callable):
        super().__init__()
        self.queue = queue
        self.stop_event = threading.Event()
        self.callback = callback

    def run(self):
        """Main thread entry point."""

        logger.info(f"Starting queue reading thread...")

        # main loop, execute until stop event is set
        while not self.stop_event.is_set():
            try:
                line = self.queue.get(block=False, timeout=1000)
                self.callback(line)
            except Exception as e:
                logger.debug(f"Error reading from queue: {str(e)}")
                continue
        logger.info(f"Stopping queue reading thread...")

    def stop(self):
        """Stop the thread."""
        if self.stop_event.is_set():
            # the thread is already stopped
            return

        logger.info(f"Stopping queue reading thread...")
        self.stop_event.set()
        self.join()
        logger.info(f"Stopped queue reading thread")


class SerialPortMonitor(threading.Thread):
    """Serial port monitor class.

    Serial port monitor is responsible for continuously monitoring
    the available serial ports and data reading threads. It will
    start a new thread for each port that is available and stop the
    threads for ports that are no longer available.
    """

    def __init__(self,
                 threads: dict,
                 ports: list[str],
                 reading_queue: queue.Queue,
                 scanning_interval: int = config.PORT_SCAN_INTERVAL):
        super().__init__()
        self.threads = threads
        self.ports = set(ports)
        self.scanning_interval = scanning_interval
        self.stop_event = threading.Event()
        self.reading_queue = reading_queue

    def run(self):
        """Main thread entry point."""
        logger.info("Starting serial port monitor...")
        while not self.stop_event.is_set():
            # scan for available ports
            logger.info("Scanning for available ports...")
            available_ports = self.scan_serial_ports(with_ping=False)
            logger.info(f"Found {len(available_ports)} available ports: {available_ports}")

            ports_to_remove = []
            # check if any ports are missing
            for port_name in self.ports:
                if port_name not in available_ports and not self.stop_event.is_set():
                    logger.info(f"Port {port_name} is no longer available, stopping the thread...")
                    self.threads[port_name].stop()
                    del self.threads[port_name]
                    ports_to_remove.append(port_name)
                    logger.info(f"Stopped the thread for port {port_name}")

            # remove the ports from the list
            for port_name in ports_to_remove:
                self.ports.remove(port_name)

            # check if any new ports are available
            for port_name in available_ports:
                if port_name not in self.ports and not self.stop_event.is_set():
                    # try to ping the port, if it fails, skip it
                    if not self.ping_port(port_name):
                        continue

                    logger.info(f"Port {port_name} is now available, starting the thread...")
                    self.ports.add(port_name)
                    stop_event = threading.Event()
                    thread = SerialPortThread(port_name, stop_event, self.reading_queue)
                    self.threads[port_name] = thread
                    thread.start()

            if not self.ports:
                logger.warning("No serial ports found.")

            if not self.stop_event.is_set():
                logger.info(f"Waiting {self.scanning_interval} seconds before scanning again...")
                sleep(self.scanning_interval)

            # Notify systemd that the service is still alive
            notify(NOTIFY_WATCHDOG)

    def scan_serial_ports(self, with_ping: bool = True) -> list[str]:
        """Scan serial ports and return a list of available ports.

        Args:
            with_ping: If True, ping the port before adding it to the list.

        """
        logger.info("Scanning serial ports...")
        ports = list_ports.grep(config.PORTS_RE)
        available_ports = []
        for port in ports:
            if self.stop_event.is_set():
                # thead is exiting, don't scan anymore
                break
            if with_ping and not self.ping_port(port.device):
                continue
            available_ports.append(port.device)
        logger.info(f"Found {len(available_ports)}\navailable ports: {available_ports}")
        return available_ports

    def ping_port(self, port_name: str) -> bool:
        """Try to open the port and read a line of data."""
        logger.info(f"Pinging port {port_name}...")
        try:
            with serial.Serial(port_name, config.BAUD_RATE, timeout=config.PORT_TIMEOUT) as ser:
                line = ser.readline().decode('utf-8').strip()
                if line:
                    logger.info(f"Successfully pinged port {port_name}")
                    return True
                else:
                    logger.warning(f"Failed to ping port {port_name}")
                    return False
        except Exception as e:
            logger.warning(f"Failed to open port {port_name}")
            return False

    def stop(self):
        """Stop the thread."""
        if self.stop_event.is_set():
            # the thread is already stopped
            return

        logger.info("Stopping serial port monitor...")
        # stop all the threads
        for port_name, thread in self.threads.items():
            logger.info(f"Stopping thread for port {port_name}...")
            thread.stop()
            logger.info(f"Stopped thread for port {port_name}")
        self.stop_event.set()
        self.join()
        logger.info("Stopped serial port monitor")


class SerialPortManager:
    """The main serial port manager class."""
    def __init__(self,
                 data_handler: callable,
                 max_retries: int = config.RECONNECT_ATTEMPTS,
                 baud_rate: int = config.BAUD_RATE,
                 timeout: int = config.PORT_TIMEOUT,
                 wait_for_ports_attempts: int = config.WAIT_FOR_PORTS_ATTEMPTS,
                 ):
        self.ports = []
        self.threads = {}
        self.stop_events = {}
        self.reading_queue = queue.Queue()
        self.max_retries = max_retries
        self.baud_rate = baud_rate
        self.timeout = timeout
        self.queue_reader = None
        self.data_handler = data_handler
        self.port_monitor = SerialPortMonitor(self.threads, self.ports, self.reading_queue)
        self.wait_fror_ports_attempts = wait_for_ports_attempts
        self.stop_event = threading.Event()

    def wait_for_serial_ports(self):
        """Waits until at least one serial port is available."""
        attempts = 0
        self.ports = self.port_monitor.scan_serial_ports()
        while not self.ports and attempts < self.wait_fror_ports_attempts:
            if self.stop_event.is_set():
                # app is exiting, don't scan anymore
                break
            logger.info("No serial ports found, waiting for 10 seconds...")
            sleep(10)
            self.ports = self.port_monitor.scan_serial_ports()
            if self.ports:
                break
            attempts += 1
        logger.info("Serial ports found, continuing...")

    def start(self):
        """Start the serial port manager."""
        logger.info("Starting serial port manager...")
        self.wait_for_serial_ports()
        if not self.ports:
            logger.error("No serial ports found, exiting...")
            sys.exit(1)
        # start the queue reading thread
        self.queue_reader = QueueReadingThread(self.reading_queue, self.data_handler)
        self.queue_reader.start()
        logger.info("Started queue reading thread")
        self.port_monitor.start()
        logger.info("Started serial port manager")

    def stop(self):
        """Stop the serial port manager."""
        logger.info("Stopping serial port manager...")
        self.stop_event.set()
        self.port_monitor.stop()
        self.queue_reader.stop()
        logger.info("Stopped serial port manager")


def simple_logging_data_handler(data: str):
    """Simple data handler that logs the data."""
    logger.info(f"Received data: {data}")


# Monitoring data example:
# {"ID":1,"co2":450,"%RH":67,"RHSP":71,"boxTempC":34,"BHSP":35,"waterTempC":42,"IHSP":40}
# Where ID is the arduino box ID

class InfluxDataLogger:
    """Serial data handler, writing to Influx DB.

    handle_data method is called for each line of data received from the serial port.
    It will deserialize the data into a dict and write it to Influx.
    """

    REQUIRED_COLUMNS = ['ID', 'co2', '%RH', 'RHSP', 'boxTempC', 'BHSP', 'waterTempC', 'IHSP']

    def __init__(
            self,
            db_host: str = config.INFLUXDB_HOST,
            db_port: int = config.INFLUXDB_PORT,
            bucket_name: str = config.INFLUXDB_BUCKET,
            token: str = config.INFLUXDB_TOKEN,
            org: str = config.INFLUXDB_ORG,
    ):
        self.db_host = db_host
        self.db_port = db_port
        self.bucket_name = bucket_name
        self.token = token
        self.org = org
        logger.info("Creating influxdb client.")
        if not self.token:
            logger.error("Influxdb token is not provided. Please put your token "
                         "into INFLUXDB_TOKEN env variable and restart the app.")
            sys.exit(1)
        self.client = InfluxDBClient(
            url=f"http://{self.db_host}:{self.db_port}",
            token=self.token,
            org=self.org
        )
        self.write_api = self.client.write_api(write_options=SYNCHRONOUS)

    def deserialize_data(self, data: str) -> dict:
        """Deserialize the data into a dict."""
        try:
            data_dict = json.loads(data)
            # validate the data
            for column in self.REQUIRED_COLUMNS:
                if column not in data_dict:
                    logger.error(f"Missing required column: {column}")
                    return None
            return data_dict
        except json.JSONDecodeError:
            logger.error(f"Invalid JSON data: {data}")
            return None

    def handle_data(self, data: str):
        """Handle the data and write to Influx."""
        data_dict = self.deserialize_data(data)
        if not data_dict:
            logger.error(f"Failed to deserialize data: {data}")
            return
        try:
            point = Point("arduino_box_data") \
                .tag("box_id", data_dict["ID"]) \
                .time(time_ns()) \
                .field("co2", data_dict["co2"]) \
                .field("humidity", data_dict["%RH"]) \
                .field("rhsp", data_dict["RHSP"]) \
                .field("box_temperature", data_dict["boxTempC"]) \
                .field("bhsp", data_dict["BHSP"]) \
                .field("water_temperature", data_dict["waterTempC"]) \
                .field("ihsp", data_dict["IHSP"])
            self.write_api.write(bucket=self.bucket_name, record=point)
            logger.info(f"Wrote data to Influx: {data_dict}")
        except Exception as e:
            logger.exception(f"Error writing data to Influx: {str(e)}")


def main():
    """Main application entry point."""

    # create the data logger, this will also create the influxdb client
    # InfluxDataLogger will exit the app if the token is not provided
    influx_data_logger = InfluxDataLogger()

    # create the serial port manager, that will be responsible for
    # monitoring available serial ports, starting and stopping the
    # threads for each port, and reading data from the ports
    port_manager = SerialPortManager(data_handler=influx_data_logger.handle_data)
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
    notify(NOTIFY_READY)


if __name__ == '__main__':
    main()
