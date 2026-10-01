include_directories(${Spring_SOURCE_DIR}/rts)
add_library(engineMetrics STATIC
		"${CMAKE_CURRENT_SOURCE_DIR}/Metrics.cpp"
	)

# PUBLIC because the metric groups that build the families live engine-side, in
# rts/Net.
target_link_libraries(engineMetrics PUBLIC prometheus-cpp::core prometheus-cpp::pull)
