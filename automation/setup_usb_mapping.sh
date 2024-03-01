# Add udev rules to map USB devices to specific device names
# Udev rules file: 99-usb-ports-nuc.rules

CONFIG_FILES_DIR="/home/erik/heimdal-box/automation"

# Copy the udev rules file to the /etc/udev/rules.d/ directory
sudo cp "$CONFIG_FILES_DIR/99-usb-ports-nuc.rules" /etc/udev/rules.d/

# Reload the udev rules
sudo udevadm control --reload-rules

# Restart the udev service
sudo udevadm trigger
