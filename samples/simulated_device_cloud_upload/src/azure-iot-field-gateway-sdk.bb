DESCRIPTION = ""
LICENSE = "MI"
SRC_URI = "file:///home/<<userName>>/azure-iot-gateway-sdk"

LIC_FILES_CHKSUM = "file:///home/<<userName>>/azure-iot-gateway-sdk/deps/iot-sdk-c/LICENSE;md5=4283671594edec4c13aeb073c219237a"

PROVIDES = "azure-iot-gateway-sdk"

DEPENDS = "glib-2.0 curl"

EXTRA_OECMAKE = "-Dinstall_executables:BOOL=ON -Drun_as_a_service:BOOL=ON -Drun_unittests:BOOL=OFF"

S = "${WORKDIR}/home/azure-iot-gateway-sdk"

SYSTEMD_SERVICE_${PN} = "simulated_device_cloud_upload.service"

FILES_${PN} = "${systemd_unitdir}/system/*"
FILES_${PN} += "${bindir}/*"
FILES_${PN} += "${libdir}/*.so"

FILES_SOLIBSDEV = ""

inherit pkgconfig cmake systemd

do_install_append() {
    # install service file
    install -d ${D}${systemd_unitdir}/system
    install -c -m 0644 ${WORKDIR}/home/azure-iot-gateway-sdk/samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload.service ${D}${systemd_unitdir}/system
    
    install -d ${D}${bindir}
    install -c -m 0644 ${WORKDIR}/home/azure-iot-gateway-sdk/samples/simulated_device_cloud_upload/src/simulated_device_cloud_upload_intel_edison.json ${D}${bindir}
}