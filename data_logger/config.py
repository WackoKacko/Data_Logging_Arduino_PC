"""Application configuration."""

import logging.handlers
import os

import sentry_sdk
from systemd.daemon import notify

# existing environment variables
INFLUXDB_HOST = os.getenv('INFLUXDB_HOST', 'localhost')
INFLUXDB_PORT = int(os.getenv('INFLUXDB_PORT', 8086))
INFLUXDB_BUCKET = os.getenv('INFLUXDB_BUCKET', 'test')
INFLUXDB_TOKEN = os.getenv('INFLUXDB_TOKEN', '')
INFLUXDB_ORG = os.getenv('INFLUXDB_ORG', 'Heimdal')
LOGGING_LEVEL = os.getenv('LOGGING_LEVEL', 'INFO')
BAUD_RATE = int(os.getenv('BAUD_RATE', 115200))
PORT_TIMEOUT = int(os.getenv('PORT_TIMEOUT', 1))
PORT_RECONNECT_DELAY = int(os.getenv('PORT_RECONNECT_DELAY', 1))
PORT_SCAN_INTERVAL = int(os.getenv('PORT_SCAN_INTERVAL', 10))
RECONNECT_ATTEMPTS = int(os.getenv('RECONNECT_ATTEMPTS', 60))
LOG_FILE_PATH = os.getenv('LOG_FILE_PATH', '/var/log/box-logger/app.log')
PORTS_RE = os.getenv('PORTS_RE', 'ttyACM*|ttyUSB*')
WAIT_FOR_PORTS_ATTEMPTS = int(os.getenv('WAIT_FOR_PORTS_ATTEMPTS', 60))
DATA_LOG_PATH = os.getenv('DATA_LOG_PATH', 'data_log')
NO_BOXES_TIMEOUT = int(os.getenv('NO_BOXES_TIMEOUT', 60* 5))  # 5 minutes

NOTIFY_READY = "READY=1"
NOTIFY_WATCHDOG = "WATCHDOG=1"
NOTIFY_SOCKET = "NOTIFY_SOCKET"

SENTRY_DSN = os.getenv('SENTRY_DSN', 'https://84ce1983583627baa346f1b2366da01b@o1409994.ingest.sentry.io/4506394675380224')

sentry_sdk.init(dsn=SENTRY_DSN)


def notify_systemd(state):
    """Send a notification to systemd."""
    if NOTIFY_SOCKET in os.environ:
        notify(state)


# create logger
logger = logging.getLogger(__name__)
logger.setLevel(LOGGING_LEVEL)

# create console handler
handler = logging.StreamHandler()
handler.setLevel(LOGGING_LEVEL)


# create formatter
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

# add formatter to handlers
handler.setFormatter(formatter)

# add handlers to logger
logger.addHandler(handler)


# make sure the log file path exists
try:
    log_file_dir = os.path.dirname(LOG_FILE_PATH)
    if not os.path.exists(log_file_dir):
        os.makedirs(log_file_dir)
    # create file handler with daily rotation
    file_handler = logging.handlers.TimedRotatingFileHandler(
        LOG_FILE_PATH, when='midnight', interval=1, backupCount=7)
    file_handler.setLevel(LOGGING_LEVEL)
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
except Exception as e:
    logger.error(f"Error creating log file directory: {str(e)}")
