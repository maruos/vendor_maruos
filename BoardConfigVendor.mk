BOARD_SEPOLICY_DIRS += \
       vendor/maru/sepolicy

BOARD_SEPOLICY_UNION += \
       service.te \
       surfaceflinger.te \
       perspectived.te \
       mflinger.te \
       mcprepare.te \
       maru_init.te \
       maru_files.te \
       file_contexts \
       service_contexts
