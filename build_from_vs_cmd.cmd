rc /fotatdylf.res src\tatdylf.rc
cl /O1 /Os /GL /GS- /Isrc src\tatdylf.cpp src\tatdylf_ui.cpp tatdylf.res kernel32.lib ws2_32.lib user32.lib shell32.lib /link /entry:entry_point /subsystem:console /fixed /merge:.rdata=.text
