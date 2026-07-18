SUMMARY = "Industrial Bridge Linux Image"
DESCRIPTION = "Minimal Linux image with Industrial Protocol Bridge application"

inherit core-image

IMAGE_INSTALL:append = " bridge-engine"

IMAGE_FEATURES += "debug-tweaks"
