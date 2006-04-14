Section "ServerFlags"
#       Option		"DontVTSwitch"	"on"
#       Option		"DontZap"	"on"
       Option		"DontZoom"	"on"
EndSection

Section "ServerLayout"
	Identifier     "single head configuration"
	Screen      0  "Screen0" 0 0
	InputDevice    "Mouse0" "CorePointer"
	InputDevice    "Keyboard0" "CoreKeyboard"
EndSection

Section "Files"
ifelse(DRIVER,NVIDIA,
	ModulePath   "/usr/lib64/xorg/modules/extensions/nvidia"
	ModulePath   "/usr/lib/xorg/modules/extensions/nvidia"
)
	ModulePath   "/usr/lib64/xorg/modules"
	ModulePath   "/usr/lib/xorg/modules"
	RgbPath      "/usr/share/X11/rgb"
	FontPath     "/usr/share/X11/fonts/misc/"
#	FontPath     "unix/:7100"
EndSection

Section "Module"
	Load  "dbe"
	Load  "extmod"
#	Load  "fbdevhw"
#	Load  "record"
	Load  "freetype"
	Load  "type1"
	Load  "dri"
	Load  "glx"
EndSection

Section "InputDevice"
	Identifier  "Keyboard0"
include(arp2-xorg.keyboard)
EndSection

Section "InputDevice"
	Identifier  "Mouse0"
include(arp2-xorg.mouse)
EndSection

Section "Monitor"
	Identifier   "Monitor0"
include(arp2-xorg.monitor)
EndSection

Section "Device"
	Identifier  "Videocard0"
`#' DRIVER graphics card detected by arp2-init.
ifelse(DRIVER,NVIDIA,
	Driver		"nvidia"
	VendorName	"NVIDIA"
	BoardName	"NVIDIA Graphics Card"
	Option		"NoLogo"
)
ifelse(DRIVER,ATI,
	Driver		"fglrx"
	VendorName	"ATI"
	BoardName	"ATI Graphics Card"
#	Option		"NoDDC"
	Option		"no_accel"	"no"
	Option		"no_dri"	"no"
	Option		"mtrr"		"off"
	Option		"UseFastTLS"	"0"
	Option		"BlockSignalsOnLock"	"on"
	Option		"UseInternalAGPGART"	"yes"
)
EndSection

Section "Screen"
	Identifier "Screen0"
	Device     "Videocard0"
	Monitor    "Monitor0"
	DefaultDepth     DEPTH
ifelse(RESOLUTION,,,
	SubSection "Display"
		Viewport   0 0
		Depth     DEPTH
		Modes    "RESOLUTION"
	EndSubSection
)
EndSection

Section "DRI"
	Group        0
	Mode         0666
EndSection
