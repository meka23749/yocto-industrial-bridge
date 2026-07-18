SUMMARY = "Industrial Protocol Bridge - Modbus TCP to MQTT"
DESCRIPTION = "Embedded application that reads industrial sensors \
via Modbus TCP and publishes data to MQTT broker. \
Includes threshold monitoring and alerting."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://bridge_engine.c \
           file://Makefile"

S = "${WORKDIR}"

do_compile() {
    oe_runmake
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 bridge-engine ${D}${bindir}/bridge-engine
}
