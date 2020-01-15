#!/bin/bash

PROJECT_ROOT="`pwd`"
echo ${PROJECT_ROOT}
MOLTENVK_PATH="${PROJECT_ROOT}/external/MoltenVK"

export VK_LAYER_PATH="${MOLTENVK_PATH}/explicit_layer.d"
export VK_ICD_FILENAMES="${MOLTENVK_PATH}/MoltenVK_icd.json"
