#!/bin/bash

rm output/outTable*; rm output/n*; ./autorun.sh manlleuA grid4_othertopos grid 4 x x

mv output/*.tar.gz output/other_topos_g4/
mv output/outTable* output/other_topos_g4/
mv output/*.png output/other_topos_g4/

rm output/outTable*; rm output/n*; ./autorun.sh manlleuA grid8_othertopos grid 8 x x

mv output/*.tar.gz output/other_topos_g8/
mv output/outTable* output/other_topos_g8/
mv output/*.png output/other_topos_g8/
