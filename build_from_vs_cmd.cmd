rc /fotatdylf.res src\tatdylf.rc
@set copts=/O1 /Os /GL /GS- /Isrc
@set lopts=/entry:entry_point /subsystem:console /fixed /merge:.rdata=.text
@set libs=kernel32.lib ws2_32.lib user32.lib shell32.lib
@set infiles=src\tatdylf.cpp src\tatdylf_ui.cpp tatdylf.res
cl %copts% %infiles% %libs% /link %lopts%
