set(component_srcs "ssd1306.c")

# get IDF version for comparison
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

		list(APPEND component_srcs)

idf_component_register(SRCS "ssd1306_i2c.c" "${component_srcs}" PRIV_REQUIRES driver INCLUDE_DIRS ".")
