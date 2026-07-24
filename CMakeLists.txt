if(CONFIG_LISM_INPUT_PROCESSOR_POINTER_ACCEL)
  target_sources(app PRIVATE src/input_processor_pointer_accel.c)
endif()

if(CONFIG_LISM_INPUT_PROCESSOR_AXIS_LOCK)
  target_sources(app PRIVATE src/input_processor_axis_lock.c)
endif()
