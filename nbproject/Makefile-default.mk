#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=ntc_10k.c faults/fault_inputs.c LCD_I2C.c eeprom_24fc04.c eev.c heatpump_control.c ntc_loops.c outputs.c main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/ntc_10k.o ${OBJECTDIR}/faults/fault_inputs.o ${OBJECTDIR}/LCD_I2C.o ${OBJECTDIR}/eeprom_24fc04.o ${OBJECTDIR}/eev.o ${OBJECTDIR}/heatpump_control.o ${OBJECTDIR}/ntc_loops.o ${OBJECTDIR}/outputs.o ${OBJECTDIR}/main.o
POSSIBLE_DEPFILES=${OBJECTDIR}/ntc_10k.o.d ${OBJECTDIR}/faults/fault_inputs.o.d ${OBJECTDIR}/LCD_I2C.o.d ${OBJECTDIR}/eeprom_24fc04.o.d ${OBJECTDIR}/eev.o.d ${OBJECTDIR}/heatpump_control.o.d ${OBJECTDIR}/ntc_loops.o.d ${OBJECTDIR}/outputs.o.d ${OBJECTDIR}/main.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/ntc_10k.o ${OBJECTDIR}/faults/fault_inputs.o ${OBJECTDIR}/LCD_I2C.o ${OBJECTDIR}/eeprom_24fc04.o ${OBJECTDIR}/eev.o ${OBJECTDIR}/heatpump_control.o ${OBJECTDIR}/ntc_loops.o ${OBJECTDIR}/outputs.o ${OBJECTDIR}/main.o

# Source Files
SOURCEFILES=ntc_10k.c faults/fault_inputs.c LCD_I2C.c eeprom_24fc04.c eev.c heatpump_control.c ntc_loops.c outputs.c main.c



CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk ${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=33CK256MP505
MP_LINKER_FILE_OPTION=,--script=p33CK256MP505.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/ntc_10k.o: ntc_10k.c  .generated_files/flags/default/5953222be7cff66e3ceaf42a9fc861e0a34bae0c .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ntc_10k.o.d 
	@${RM} ${OBJECTDIR}/ntc_10k.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ntc_10k.c  -o ${OBJECTDIR}/ntc_10k.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ntc_10k.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/faults/fault_inputs.o: faults/fault_inputs.c  .generated_files/flags/default/77abd6d763cb29856ca81766b0fde222a36980ac .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/faults" 
	@${RM} ${OBJECTDIR}/faults/fault_inputs.o.d 
	@${RM} ${OBJECTDIR}/faults/fault_inputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  faults/fault_inputs.c  -o ${OBJECTDIR}/faults/fault_inputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/faults/fault_inputs.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/LCD_I2C.o: LCD_I2C.c  .generated_files/flags/default/c2c57aafe7ad1919c56cf53cf0c1f34edebc8c6f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/LCD_I2C.o.d 
	@${RM} ${OBJECTDIR}/LCD_I2C.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  LCD_I2C.c  -o ${OBJECTDIR}/LCD_I2C.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/LCD_I2C.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_24fc04.o: eeprom_24fc04.c  .generated_files/flags/default/bfde6437f553d01818306f607e1b054d817cc145 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_24fc04.o.d 
	@${RM} ${OBJECTDIR}/eeprom_24fc04.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_24fc04.c  -o ${OBJECTDIR}/eeprom_24fc04.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_24fc04.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eev.o: eev.c  .generated_files/flags/default/ab7f8f95f21ba00772844a82928b04dcfa268d7e .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eev.o.d 
	@${RM} ${OBJECTDIR}/eev.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eev.c  -o ${OBJECTDIR}/eev.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eev.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/heatpump_control.o: heatpump_control.c  .generated_files/flags/default/e2e55c8c475f2eb61600fa832883316ed29b70a2 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/heatpump_control.o.d 
	@${RM} ${OBJECTDIR}/heatpump_control.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  heatpump_control.c  -o ${OBJECTDIR}/heatpump_control.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/heatpump_control.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ntc_loops.o: ntc_loops.c  .generated_files/flags/default/34851196bb9553cb267722b7eaa5e5abd24d38fb .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ntc_loops.o.d 
	@${RM} ${OBJECTDIR}/ntc_loops.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ntc_loops.c  -o ${OBJECTDIR}/ntc_loops.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ntc_loops.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/outputs.o: outputs.c  .generated_files/flags/default/3c2710bd48e0124f6948d16e029c83f01c61114 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/outputs.o.d 
	@${RM} ${OBJECTDIR}/outputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  outputs.c  -o ${OBJECTDIR}/outputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/outputs.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/65a0e5d115ba1bcff1944e8de1186ea9453be82f .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -mno-eds-warn  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/ntc_10k.o: ntc_10k.c  .generated_files/flags/default/ee171e290c53fc3a42de31e91bd1abefd32af192 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ntc_10k.o.d 
	@${RM} ${OBJECTDIR}/ntc_10k.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ntc_10k.c  -o ${OBJECTDIR}/ntc_10k.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ntc_10k.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/faults/fault_inputs.o: faults/fault_inputs.c  .generated_files/flags/default/6b29bb32d17426ff16be144945fb34cd90421e91 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}/faults" 
	@${RM} ${OBJECTDIR}/faults/fault_inputs.o.d 
	@${RM} ${OBJECTDIR}/faults/fault_inputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  faults/fault_inputs.c  -o ${OBJECTDIR}/faults/fault_inputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/faults/fault_inputs.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/LCD_I2C.o: LCD_I2C.c  .generated_files/flags/default/2352cd8992a00f049614293930062f3ca9dab02b .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/LCD_I2C.o.d 
	@${RM} ${OBJECTDIR}/LCD_I2C.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  LCD_I2C.c  -o ${OBJECTDIR}/LCD_I2C.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/LCD_I2C.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eeprom_24fc04.o: eeprom_24fc04.c  .generated_files/flags/default/d2e3b2c133d5ebc6795e6b504dfe46cd1c6c5090 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eeprom_24fc04.o.d 
	@${RM} ${OBJECTDIR}/eeprom_24fc04.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eeprom_24fc04.c  -o ${OBJECTDIR}/eeprom_24fc04.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eeprom_24fc04.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/eev.o: eev.c  .generated_files/flags/default/6e044d621871e582b4fc7ba04f61aba6563661fc .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/eev.o.d 
	@${RM} ${OBJECTDIR}/eev.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  eev.c  -o ${OBJECTDIR}/eev.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/eev.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/heatpump_control.o: heatpump_control.c  .generated_files/flags/default/6642e64dc8f739d0bbc417c35568a3af67726dc0 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/heatpump_control.o.d 
	@${RM} ${OBJECTDIR}/heatpump_control.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  heatpump_control.c  -o ${OBJECTDIR}/heatpump_control.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/heatpump_control.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ntc_loops.o: ntc_loops.c  .generated_files/flags/default/edb8dbbd4f680c236e973311cbe11a4c34d15317 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/ntc_loops.o.d 
	@${RM} ${OBJECTDIR}/ntc_loops.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ntc_loops.c  -o ${OBJECTDIR}/ntc_loops.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ntc_loops.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/outputs.o: outputs.c  .generated_files/flags/default/614bf89d73aabada9e05827fcc1c297831d89df6 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/outputs.o.d 
	@${RM} ${OBJECTDIR}/outputs.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  outputs.c  -o ${OBJECTDIR}/outputs.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/outputs.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/default/6c913162c3b7eae00c1e01f3f307019e8f98d571 .generated_files/flags/default/da39a3ee5e6b4b0d3255bfef95601890afd80709
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -O0 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o ${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG -D__MPLAB_DEBUGGER_PK5=1  -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)   -mreserve=data@0x1000:0x101B -mreserve=data@0x101C:0x101D -mreserve=data@0x101E:0x101F -mreserve=data@0x1020:0x1021 -mreserve=data@0x1022:0x1023 -mreserve=data@0x1024:0x1027 -mreserve=data@0x1028:0x104F   -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,--defsym=__MPLAB_DEBUGGER_PK5=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	
else
${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} ${DISTDIR} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o ${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,${DISTDIR}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	${MP_CC_DIR}\\xc16-bin2hex ${DISTDIR}/Heat_Pump_DHWT.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf   -mdfp="${DFP_DIR}/xc16" 
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${OBJECTDIR}
	${RM} -r ${DISTDIR}

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(wildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
