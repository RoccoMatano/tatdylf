cl /O1 /Os /GL /Isrc /DTATDYLF_READ_LEASE_TIME src\tatdylf.cpp src\tatdylf_ui.cpp kernel32.lib ws2_32.lib user32.lib shell32.lib /link /entry:entry_point /subsystem:console
