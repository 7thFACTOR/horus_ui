cmake_minimum_required(VERSION 2.8)
project(stbi)
set (STBI_INCLUDE
	./stb_image.h
	./stb_image.c
	./stb_image_write.h
	./stb_image_write.c
)

add_library(stbi STATIC ${STBI_INCLUDE})

set_target_properties(stbi PROPERTIES DEBUG_POSTFIX _d OUTPUT_NAME "stbi")

INSTALL_TARGETS(/lib stbi)
INSTALL_FILES(/include FILES ${STBI_INCLUDE})

if (APPLE AND IOS)
	# cmake bug in ios installation
	# http://www.cmake.org/Bug/view.php?id=12506
	INSTALL_FILES(/lib FILES ${PROJECT_BINARY_DIR}/Debug${IOS_INSTALL_POSTFIX}/libstbi_d.a)

	# target properties
	set_target_properties(stbi PROPERTIES
		XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IOS_VERSION}
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
	)
endif()