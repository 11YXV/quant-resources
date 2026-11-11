#!/bin/bash
# Rebuild DiversifiedSuite_NQ_MNQ.cpp from its parts (run from this folder). The header holds measured figures:
# update it from the mock run before committing (lessons #98).
cat header.txt body_engine.inc body_orders.inc body_study.inc > ../../DiversifiedSuite_NQ_MNQ.cpp
