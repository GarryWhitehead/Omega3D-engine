#!/bin/bash

mydir="${0%/*}"
MOLTENVK_PATH="${mydir}/external/MoltenVK"
echo ${MOLTENVK_PATH}
export VK_LAYER_PATH="${MOLTENVK_PATH}/explicit_layer.d"
export VK_ICD_FILENAMES="${MOLTENVK_PATH}/MoltenVK_icd.json"

