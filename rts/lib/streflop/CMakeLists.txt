cmake_minimum_required(VERSION 3.10)
project(streflop C CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(STREFLOP_AUTO "Autodetect the most appropriate Streflop implementation" ON)
option(STREFLOP_SSE "Use SSE instead of x87 extended precision" OFF)
option(STREFLOP_NEON "Use ARM NEON (auto-enabled on ARM64 when SSE/SOFT are off)" OFF)
option(STREFLOP_SOFT "Use soft float" OFF)
option(STREFLOP_NO_DENORMALS "Squash denormals to zero" OFF)
option(BUILD_TESTS "Build test executables" OFF)

# Autodetect STREFLOP mode when STREFLOP_AUTO is ON and no explicit mode is selected
if(STREFLOP_AUTO AND NOT STREFLOP_SSE AND NOT STREFLOP_NEON AND NOT STREFLOP_SOFT AND NOT STREFLOP_NO_DENORMALS AND NOT BUILD_TESTS)
	# Use HOST processor as fallback when cross-compiling (CMAKE_SYSTEM_PROCESSOR may be empty)
	if(NOT CMAKE_SYSTEM_PROCESSOR AND CMAKE_HOST_SYSTEM_PROCESSOR)
		set(_streflop_processor "${CMAKE_HOST_SYSTEM_PROCESSOR}")
	else()
		set(_streflop_processor "${CMAKE_SYSTEM_PROCESSOR}")
	endif()
	if(_streflop_processor MATCHES "arm64|aarch64|armv8|ARM64|AARCH64")
		set(STREFLOP_NEON ON CACHE BOOL "Use ARM NEON (autodetected)" FORCE)
		message(STATUS "STREFLOP: Autodetected ARM64 platform - enabling NEON mode")
	elseif(_streflop_processor MATCHES "x86|i686|amd64|AMD64|x86_64")
		set(STREFLOP_SSE ON CACHE BOOL "Use SSE (autodetected)" FORCE)
		message(STATUS "STREFLOP: Autodetected x86/x86_64 platform - enabling SSE mode")
	else()
		message(FATAL_ERROR "STREFLOP: Unknown architecture '${_streflop_processor}', cannot proceed with autodetection")
	endif()
endif()

# Float-32 source files
SET(libm_flt32_source
	"libm/flt-32/e_acosf.cpp"
	"libm/flt-32/e_acoshf.cpp"
	"libm/flt-32/e_asinf.cpp"
	"libm/flt-32/e_atan2f.cpp"
	"libm/flt-32/e_atanhf.cpp"
	"libm/flt-32/e_coshf.cpp"
	"libm/flt-32/e_exp2f.cpp"
	"libm/flt-32/e_expf.cpp"
	"libm/flt-32/e_fmodf.cpp"
	"libm/flt-32/e_gammaf_r.cpp"
	"libm/flt-32/e_hypotf.cpp"
	"libm/flt-32/e_j0f.cpp"
	"libm/flt-32/e_j1f.cpp"
	"libm/flt-32/e_jnf.cpp"
	"libm/flt-32/e_lgammaf_r.cpp"
	"libm/flt-32/e_log10f.cpp"
	"libm/flt-32/e_log2f.cpp"
	"libm/flt-32/e_logf.cpp"
	"libm/flt-32/e_powf.cpp"
	"libm/flt-32/e_rem_pio2f.cpp"
	"libm/flt-32/e_remainderf.cpp"
	"libm/flt-32/e_sinhf.cpp"
	"libm/flt-32/e_sqrtf.cpp"
	"libm/flt-32/k_cosf.cpp"
	"libm/flt-32/k_rem_pio2f.cpp"
	"libm/flt-32/k_sinf.cpp"
	"libm/flt-32/k_tanf.cpp"
	"libm/flt-32/s_asinhf.cpp"
	"libm/flt-32/s_atanf.cpp"
	"libm/flt-32/s_cbrtf.cpp"
	"libm/flt-32/s_ceilf.cpp"
	"libm/flt-32/s_copysignf.cpp"
	"libm/flt-32/s_cosf.cpp"
	"libm/flt-32/s_erff.cpp"
	"libm/flt-32/s_expm1f.cpp"
	"libm/flt-32/s_fabsf.cpp"
	"libm/flt-32/s_finitef.cpp"
	"libm/flt-32/s_floorf.cpp"
	"libm/flt-32/s_fpclassifyf.cpp"
	"libm/flt-32/s_frexpf.cpp"
	"libm/flt-32/s_ilogbf.cpp"
	"libm/flt-32/s_isinff.cpp"
	"libm/flt-32/s_isnanf.cpp"
	"libm/flt-32/s_ldexpf.cpp"
	"libm/flt-32/s_llrintf.cpp"
	"libm/flt-32/s_llroundf.cpp"
	"libm/flt-32/s_log1pf.cpp"
	"libm/flt-32/s_logbf.cpp"
	"libm/flt-32/s_lrintf.cpp"
	"libm/flt-32/s_lroundf.cpp"
	"libm/flt-32/s_modff.cpp"
	"libm/flt-32/s_nearbyintf.cpp"
	"libm/flt-32/s_nextafterf.cpp"
	"libm/flt-32/s_remquof.cpp"
	"libm/flt-32/s_rintf.cpp"
	"libm/flt-32/s_roundf.cpp"
	"libm/flt-32/s_scalblnf.cpp"
	"libm/flt-32/s_scalbnf.cpp"
	"libm/flt-32/s_signbitf.cpp"
	"libm/flt-32/s_sincosf.cpp"
	"libm/flt-32/s_sinf.cpp"
	"libm/flt-32/s_tanf.cpp"
	"libm/flt-32/s_tanhf.cpp"
	"libm/flt-32/s_truncf.cpp"
	"libm/flt-32/w_expf.cpp"
)

# Double-64 source files
SET(libm_dbl64_source
	"libm/dbl-64/branred.cpp"
	"libm/dbl-64/doasin.cpp"
	"libm/dbl-64/dosincos.cpp"
	"libm/dbl-64/e_acos.cpp"
	"libm/dbl-64/e_acosh.cpp"
	"libm/dbl-64/e_asin.cpp"
	"libm/dbl-64/e_atan2.cpp"
	"libm/dbl-64/e_atanh.cpp"
	"libm/dbl-64/e_cosh.cpp"
	"libm/dbl-64/e_exp.cpp"
	"libm/dbl-64/e_exp2.cpp"
	"libm/dbl-64/e_fmod.cpp"
	"libm/dbl-64/e_gamma_r.cpp"
	"libm/dbl-64/e_hypot.cpp"
	"libm/dbl-64/e_j0.cpp"
	"libm/dbl-64/e_j1.cpp"
	"libm/dbl-64/e_jn.cpp"
	"libm/dbl-64/e_lgamma_r.cpp"
	"libm/dbl-64/e_log.cpp"
	"libm/dbl-64/e_log10.cpp"
	"libm/dbl-64/e_log2.cpp"
	"libm/dbl-64/e_pow.cpp"
	"libm/dbl-64/e_rem_pio2.cpp"
	"libm/dbl-64/e_remainder.cpp"
	"libm/dbl-64/e_sinh.cpp"
	"libm/dbl-64/e_sqrt.cpp"
	"libm/dbl-64/halfulp.cpp"
	"libm/dbl-64/k_cos.cpp"
	"libm/dbl-64/k_rem_pio2.cpp"
	"libm/dbl-64/k_sin.cpp"
	"libm/dbl-64/k_tan.cpp"
	"libm/dbl-64/mpa.cpp"
	"libm/dbl-64/mpatan.cpp"
	"libm/dbl-64/mpatan2.cpp"
	"libm/dbl-64/mpexp.cpp"
	"libm/dbl-64/mplog.cpp"
	"libm/dbl-64/mpsqrt.cpp"
	"libm/dbl-64/mptan.cpp"
	"libm/dbl-64/s_asinh.cpp"
	"libm/dbl-64/s_atan.cpp"
	"libm/dbl-64/s_cbrt.cpp"
	"libm/dbl-64/s_ceil.cpp"
	"libm/dbl-64/s_copysign.cpp"
	"libm/dbl-64/s_cos.cpp"
	"libm/dbl-64/s_erf.cpp"
	"libm/dbl-64/s_expm1.cpp"
	"libm/dbl-64/s_fabs.cpp"
	"libm/dbl-64/s_finite.cpp"
	"libm/dbl-64/s_floor.cpp"
	"libm/dbl-64/s_fpclassify.cpp"
	"libm/dbl-64/s_frexp.cpp"
	"libm/dbl-64/s_ilogb.cpp"
	"libm/dbl-64/s_isinf.cpp"
	"libm/dbl-64/s_isnan.cpp"
	"libm/dbl-64/s_ldexp.cpp"
	"libm/dbl-64/s_llrint.cpp"
	"libm/dbl-64/s_llround.cpp"
	"libm/dbl-64/s_log1p.cpp"
	"libm/dbl-64/s_logb.cpp"
	"libm/dbl-64/s_lrint.cpp"
	"libm/dbl-64/s_lround.cpp"
	"libm/dbl-64/s_modf.cpp"
	"libm/dbl-64/s_nearbyint.cpp"
	"libm/dbl-64/s_nextafter.cpp"
	"libm/dbl-64/s_nexttoward.cpp"
	"libm/dbl-64/s_remquo.cpp"
	"libm/dbl-64/s_rint.cpp"
	"libm/dbl-64/s_round.cpp"
	"libm/dbl-64/s_scalbln.cpp"
	"libm/dbl-64/s_scalbn.cpp"
	"libm/dbl-64/s_signbit.cpp"
	"libm/dbl-64/s_sin.cpp"
	"libm/dbl-64/s_sincos.cpp"
	"libm/dbl-64/s_tan.cpp"
	"libm/dbl-64/s_tanh.cpp"
	"libm/dbl-64/s_trunc.cpp"
	"libm/dbl-64/sincos32.cpp"
	"libm/dbl-64/slowexp.cpp"
	"libm/dbl-64/slowpow.cpp"
	"libm/dbl-64/w_exp.cpp"
)

# Long double-96 source files (not used when STREFLOP_SSE is enabled)
SET(libm_ldbl96_source
	"libm/ldbl-96/e_acoshl.cpp"
	"libm/ldbl-96/e_asinl.cpp"
	"libm/ldbl-96/e_atan2l.cpp"
	"libm/ldbl-96/e_atanhl.cpp"
	"libm/ldbl-96/e_coshl.cpp"
	"libm/ldbl-96/e_gammal_r.cpp"
	"libm/ldbl-96/e_hypotl.cpp"
	"libm/ldbl-96/e_j0l.cpp"
	"libm/ldbl-96/e_j1l.cpp"
	"libm/ldbl-96/e_jnl.cpp"
	"libm/ldbl-96/e_lgammal_r.cpp"
	"libm/ldbl-96/e_remainderl.cpp"
	"libm/ldbl-96/e_sinhl.cpp"
	"libm/ldbl-96/s_asinhl.cpp"
	"libm/ldbl-96/s_cbrtl.cpp"
	"libm/ldbl-96/s_ceill.cpp"
	"libm/ldbl-96/s_copysignl.cpp"
	"libm/ldbl-96/s_cosl.cpp"
	"libm/ldbl-96/s_erfl.cpp"
	"libm/ldbl-96/s_fabsl.cpp"
	"libm/ldbl-96/s_finitel.cpp"
	"libm/ldbl-96/s_floorl.cpp"
	"libm/ldbl-96/s_fpclassifyl.cpp"
	"libm/ldbl-96/s_frexpl.cpp"
	"libm/ldbl-96/s_ilogbl.cpp"
	"libm/ldbl-96/s_isinfl.cpp"
	"libm/ldbl-96/s_isnanl.cpp"
	"libm/ldbl-96/s_ldexpl.cpp"
	"libm/ldbl-96/s_llrintl.cpp"
	"libm/ldbl-96/s_llroundl.cpp"
	"libm/ldbl-96/s_logbl.cpp"
	"libm/ldbl-96/s_lrintl.cpp"
	"libm/ldbl-96/s_lroundl.cpp"
	"libm/ldbl-96/s_modfl.cpp"
	"libm/ldbl-96/s_nearbyintl.cpp"
	"libm/ldbl-96/s_nextafterl.cpp"
	"libm/ldbl-96/s_remquol.cpp"
	"libm/ldbl-96/s_rintl.cpp"
	"libm/ldbl-96/s_roundl.cpp"
	"libm/ldbl-96/s_scalblnl.cpp"
	"libm/ldbl-96/s_scalbnl.cpp"
	"libm/ldbl-96/s_signbitl.cpp"
	"libm/ldbl-96/s_sincosl.cpp"
	"libm/ldbl-96/s_sinl.cpp"
	"libm/ldbl-96/s_tanhl.cpp"
	"libm/ldbl-96/s_tanl.cpp"
	"libm/ldbl-96/s_truncl.cpp"
	"libm/ldbl-96/w_expl.cpp"
)

# MSVC 64-bit requires an ASM file for FPU settings
SET(fpu_settings_asm "")
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	ENABLE_LANGUAGE(ASM_MASM)
	SET(fpu_settings_asm "FPUSettings.asm")
endif()

# Compile flags for libm sources only: include path and warning suppression.
# Optimization and fpmath flags are set via target_compile_options and apply
# to all sources (including libm) uniformly.
SET(streflop_cxx_flags "-I\"${CMAKE_CURRENT_SOURCE_DIR}/libm/headers\"")
if(NOT MSVC)
	SET(streflop_cxx_flags "${streflop_cxx_flags} -w -Wno-narrowing")
endif()

# Set compile flags for libm sources.
# -ffp-contract=off prevents FMA fusion (fmadd/fmsub) in transcendental functions;
if(MSVC)
	SET(libm_extra_flags "/fp:strict")
else()
	SET(libm_extra_flags "-ffp-contract=off")
endif()

SET_SOURCE_FILES_PROPERTIES(${libm_flt32_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_FLT32 ${streflop_cxx_flags} ${libm_extra_flags}")
SET_SOURCE_FILES_PROPERTIES(${libm_dbl64_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_DBL64 ${streflop_cxx_flags} ${libm_extra_flags}")

# Conditionally add long double sources (not for SSE or NEON mode — no x87 extended precision)
if(NOT STREFLOP_SSE AND NOT STREFLOP_NEON)
	SET_SOURCE_FILES_PROPERTIES(${libm_ldbl96_source} PROPERTIES COMPILE_FLAGS "-DLIBM_COMPILING_LDBL96 ${streflop_cxx_flags} ${libm_extra_flags}")
	SET(full_libm_source ${libm_flt32_source} ${libm_dbl64_source} ${libm_ldbl96_source})
else()
	SET(full_libm_source ${libm_flt32_source} ${libm_dbl64_source})
endif()

# Core library source files
SET(streflop_core_source
	SMath.cpp
	Random.cpp
	${fpu_settings_asm}
)

# SoftFloat wrapper sources (conditionally added)
if(STREFLOP_SOFT)
	# Generate specialized SoftFloat wrapper files
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SoftFloatWrapper.cpp
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperSimple.cpp COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SoftFloatWrapper.cpp
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperDouble.cpp COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/SoftFloatWrapper.cpp
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperExtended.cpp COPYONLY)

	# Set compile flags for specialized SoftFloat wrappers
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperSimple.cpp
		PROPERTIES COMPILE_FLAGS "-DN_SPECIALIZED=32")
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperDouble.cpp
		PROPERTIES COMPILE_FLAGS "-DN_SPECIALIZED=64")
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperExtended.cpp
		PROPERTIES COMPILE_FLAGS "-DN_SPECIALIZED=96")

	SET(streflop_core_source ${streflop_core_source}
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperSimple.cpp
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperDouble.cpp
		${CMAKE_CURRENT_BINARY_DIR}/SoftFloatWrapperExtended.cpp
		softfloat/softfloat.cpp)

	# Suppress warnings in third-party softfloat code
	if(NOT MSVC)
		SET_SOURCE_FILES_PROPERTIES(softfloat/softfloat.cpp PROPERTIES COMPILE_FLAGS "-w")
	endif()
endif()

# Create the static library
ADD_LIBRARY(streflop STATIC EXCLUDE_FROM_ALL
	${streflop_core_source}
	${full_libm_source}
)

# Include directories
target_include_directories(streflop PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Set FPU type compile definitions and flags
if(STREFLOP_SOFT)
	target_compile_definitions(streflop PUBLIC STREFLOP_SOFT)
elseif(STREFLOP_SSE)
	target_compile_definitions(streflop PUBLIC STREFLOP_SSE)
	if(NOT MSVC)
		target_compile_options(streflop PRIVATE -mfpmath=sse -msse -msse2)
	endif()
elseif(STREFLOP_NEON)
	target_compile_definitions(streflop PUBLIC STREFLOP_NEON)
	if(NOT MSVC)
		target_compile_options(streflop PRIVATE -march=armv8-a+simd)
	endif()
else()
	# Default to X87 mode
	target_compile_definitions(streflop PUBLIC STREFLOP_X87)
	if(NOT MSVC)
		# X87 mode: use 387 FPU
		# Force IEEE 754 compliance by storing to memory after each FPU operation
		target_compile_options(streflop PRIVATE -mfpmath=387 -ffloat-store)
	endif()
endif()

# Add NO_DENORMALS option if enabled
if(STREFLOP_NO_DENORMALS)
	target_compile_definitions(streflop PUBLIC STREFLOP_NO_DENORMALS)
endif()

# Set library properties
if(NOT MSVC)
	# Use PIC flag for position independent code
	set_target_properties(streflop PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(BUILD_TESTS)
	add_executable(arithmeticTest tests/arithmeticTest.cpp)
	target_link_libraries(arithmeticTest streflop)
	
	add_executable(randomTest tests/randomTest.cpp)
	target_link_libraries(randomTest streflop)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
  set(STREFLOP_STANDALONE ${PROJECT_IS_TOP_LEVEL})
else()
  if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(STREFLOP_STANDALONE TRUE)
  else()
    set(STREFLOP_STANDALONE FALSE)
  endif()
endif()

if(STREFLOP_STANDALONE)
  install(TARGETS streflop ARCHIVE DESTINATION lib)
  install(FILES
    SMath.h
    Random.h
    streflop.h
    FPUSettings.h
    IntegerTypes.h
    System.h
    X87DenormalSquasher.h
    SoftFloatWrapper.h
    DESTINATION include/streflop
  )
endif()