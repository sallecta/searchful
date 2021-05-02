#Project variables and build targets

##Targets
Debug_Mint64
$(TARGET_OUTPUT_DIR)
$(TARGET_NAME)

##Output filename
$(WORKSPACEDIR)/!out/bin/$(TARGET_NAME)/$(PROJECTNAME)

##Objects output dir
$(WORKSPACEDIR)/!out/obj/$(TARGET_NAME)/$(PROJECTNAME)


#Project compiler search directories
/usr/include/gtk-2.0
/usr/lib/x86_64-linux-gnu/gtk-2.0/include
/usr/include/glib-2.0
/usr/lib/x86_64-linux-gnu/glib-2.0/include
/usr/include/cairo
/usr/include/pango-1.0
/usr/include/harfbuzz
/usr/include/gdk-pixbuf-2.0
/usr/include/atk-1.0
/usr/include/poppler/glib

#Dependencies linking.
libgtk-x11-2.0
libgdk-x11-2.0
libpango-1.0
libglib-2.0
libgdk_pixbuf-2.0
