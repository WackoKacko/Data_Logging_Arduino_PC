#!/bin/bash

# Variables
REPO_URL="https://github.com/WackoKacko/Data_Logging_Arduino_PC"
CONFIG_FILES_DIR="/home/erik/heimdal-box/automation/arduino_code"
ARDUINO_CLI_PATH="/home/erik/bin/arduino-cli"  # Assuming default installation
BAUD_RATE=115200
PYTHON_PATH="/home/erik/heimdal-env/bin/python"
MONITORING_SCRIPT_PATH="/home/erik/heimdal-box/automation/port_monitor.py"
BOARD_IDENTIFIER="arduino:megaavr:nona4809"
CHECKOUT_DIR="/home/erik/heimdal-box/automation/arduino_code"
BRANCH="main"


# Functions

function checkout_branch() {
    # Change to the directory
    cd "$CHECKOUT_DIR"

    # If there are any local changes, discard them
    git -C "$CHECKOUT_DIR" reset --hard

    # Remove all untracked files, if any
    git -C "$CHECKOUT_DIR" clean -fd

    echo "Checking out the branch..."
    if git -C "$CHECKOUT_DIR" checkout "$BRANCH"; then
        echo "Successfully checked out the branch."
    else
        echo "Failed to checkout the branch. Please check the branch name."
        exit 1
    fi
}

function pull_code_from_github() {
    echo "Starting to pull the code from GitHub..."
    # Check if the directory exists
    if [ -d "$CHECKOUT_DIR" ]; then
        checkout_branch
        # Pull the latest code, overwriting any local changes
        if git -C "$CHECKOUT_DIR" pull -f; then
            echo "Successfully pulled the code from GitHub."
        else
            echo "Failed to pull the code from GitHub. Please check the repository URL."
            exit 1
        fi
    else
      if git clone "$REPO_URL" "$CHECKOUT_DIR"; then
          echo "Successfully pulled the code from GitHub."
          checkout_branch
      else
          echo "Failed to pull the code from GitHub. Please check the repository URL."
          exit 1
      fi
    fi
}

function get_config_file() {
    local box_name=$1
    local config_file="${CONFIG_FILES_DIR}/${box_name}_params.conf"

    echo "Retrieving config file: ${config_file}"
    if [ -f "$config_file" ]; then
        echo "Config file found."
    else
        echo "Config file not found. Please check the file name and location."
        exit 1
    fi
}

function substitute_parameters() {
    local box_name=$1
    local config_file="${CONFIG_FILES_DIR}/${box_name}_params.conf"
    local ino_file="frasers_controller/frasers_controller.ino"

    echo "Substituting parameters in ${ino_file}"

    sed -i "s/\\$\\\$DEVICE_ID\\$\\$/$(grep DEVICE_ID ${config_file} | cut -d '=' -f2)/g" ${ino_file}
    sed -i "s/\\$\\\$TEMP_MAX\\$\\$/$(grep TEMP_MAX ${config_file} | cut -d '=' -f2)/g" ${ino_file}
    sed -i "s/\\$\\\$TEMP_MIN\\$\\$/$(grep TEMP_MIN ${config_file} | cut -d '=' -f2)/g" ${ino_file}
    sed -i "s/\\$\\\$RH_MAX\\$\\$/$(grep RH_MAX ${config_file} | cut -d '=' -f2)/g" ${ino_file}
    sed -i "s/\\$\\\$RH_MIN\\$\\$/$(grep RH_MIN ${config_file} | cut -d '=' -f2)/g" ${ino_file}

    echo "Parameter substitution completed."
}

function build_firmware() {
    echo "Building the Arduino firmware..."
    if sudo $ARDUINO_CLI_PATH compile --fqbn $BOARD_IDENTIFIER frasers_controller; then
        echo "Firmware build successful."
    else
        echo "Firmware build failed. Please check Arduino CLI output for errors."
        exit 1
    fi
}

function flash_firmware() {
    local serial_port=$1

    echo "Flashing firmware to ${serial_port}..."
    echo "Stopping the data logger service..."
    sudo systemctl stop BoxDataLogger.service
    echo "Data logger service stopped."
    if sudo $ARDUINO_CLI_PATH upload -vvvv -p $(readlink -f ${serial_port}) --fqbn $BOARD_IDENTIFIER frasers_controller; then
        echo "Firmware flash successful."
        echo "Starting the data logger service..."
        sudo systemctl start BoxDataLogger.service
        echo "Data logger service started."
    else
        echo "Firmware flash failed. Please check Arduino CLI output for errors."
        echo "Starting the data logger service..."
        sudo systemctl start BoxDataLogger.service
        echo "Data logger service started."
        exit 1
    fi
}

function monitor_serial_port() {
    local serial_port=$1

    echo "Monitoring serial port ${serial_port}..."
    if sudo $PYTHON_PATH $MONITORING_SCRIPT_PATH $serial_port $BAUD_RATE; then
        echo "Data received from the box succesfully! Monitoring completed."
        echo "Deployment completed successfully! $box_name is now ready to log data."
    else
        echo "Failed to receive data or monitoring timed out."
    fi
}

# Main Script Logic
if [ $# -ne 1 ]; then
    echo "Usage: ./deploy.sh <box_name>"
    exit 1
fi

box_name=$1

pull_code_from_github
get_config_file $box_name
substitute_parameters $box_name
build_firmware
flash_firmware "/dev/${box_name}"
monitor_serial_port "/dev/${box_name}"
