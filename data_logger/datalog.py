from abc import abstractmethod, ABCMeta
from time import time_ns

import json
import logging.handlers
import os
import sys


from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import config
from config import logger


# Monitoring data example:
# {"ID":1,"co2":450,"%RH":67,"RHSP":71,"boxTempC":34,"BHSP":35,"waterTempC":42,"IHSP":40}
# Where ID is the arduino box ID


def simple_logging_data_handler(data: str):
    """Simple data handler that logs the data."""
    logger.info(f"Received data: {data}")


class DataLoggerBase(metaclass=ABCMeta):
    """Base class for data loggers.

    You can create your own data logger by inheriting from this class and implementing
    the log_results method.

    """

    REQUIRED_COLUMNS = ['ID', 'co2', '%RH', 'RHSP', 'boxTempC', 'BHSP', 'waterTempC', 'IHSP']

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
        """Handle the data."""
        data_dict = self.deserialize_data(data)
        if not data_dict:
            logger.warning(f"Failed to deserialize data: {data}")
            return
        try:
            self.log_results(data_dict)
        except Exception as e:
            logger.exception(f"Error logging data: {str(e)}")

    @abstractmethod
    def log_results(self, data: dict):
        pass


class InfluxDataLogger(DataLoggerBase):
    """Serial data handler, writing to Influx DB.

    handle_data method is called for each line of data received from the serial port.
    It will deserialize the data into a dict and write it to Influx.
    """

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

    def log_results(self, data: dict):
        """Write the results to influx"""
        point = Point("arduino_box_data") \
            .tag("box_id", data["ID"]) \
            .time(time_ns()) \
            .field("co2", data["co2"]) \
            .field("humidity", data["%RH"]) \
            .field("rhsp", data["RHSP"]) \
            .field("box_temperature", data["boxTempC"]) \
            .field("bhsp", data["BHSP"]) \
            .field("water_temperature", data["waterTempC"]) \
            .field("ihsp", data["IHSP"])
        self.write_api.write(bucket=self.bucket_name, record=point)
        logger.info(f"Wrote data to Influx: {data}")


class RotatingFileDataLogger(DataLoggerBase):
    """Serial data handler, writing to a rotating file.

    handle_data method is called for each line of data received from the serial port.
    It will deserialize the data into a dict and write it to a file.

    """

    FILE_NAME = "data.log"

    def __init__(self, log_path: str = config.DATA_LOG_PATH):
        logger.info(f"Creating file data logger, log path: {log_path}")
        self.log_path = log_path
        if not os.path.exists(self.log_path):
            os.makedirs(self.log_path)
        self.file_path = os.path.join(self.log_path, self.FILE_NAME)
        self.data_logger = logging.getLogger(__name__)
        self.data_logger.setLevel(logging.INFO)
        # create file handler with daily rotation
        file_handler = logging.handlers.TimedRotatingFileHandler(
            self.file_path, when='midnight', interval=1, backupCount=7)
        file_handler.setLevel(logging.INFO)
        # create formatter, to write the data as json
        formatter = logging.Formatter('%(message)s')
        file_handler.setFormatter(formatter)
        self.data_logger.addHandler(file_handler)
        logger.info(f"File data logger created, log file path: {self.file_path}")

    def log_results(self, data: dict):
        """Write the results to a file."""
        data["ts"] = time_ns()
        self.data_logger.info(json.dumps(data))
