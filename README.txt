README.txt

Andrew MacKie-Mason
CMSC 162, University of Chicago
http://brick.cs.uchicago.edu/Courses/CMSC-16200/2012/pmwiki/pmwiki.php/Student/Diplomacy

Diplomacy

Currently the only functional part of this project is the game logic and move resolver. To test it, run:

./diplomacy "configfile.dip" "orderfile1.dip" "orderfile2.dip" ...

configs/newgame.dip is a standard set up for the beginning of a game of diplomacy

Various tests correspond to the diagrams in diplomacy.pdf (the rule) to test that it functions properly. Some tests that can be run:

Diagram 4:
./diplomacy configs/d4config.dip orders/d4ger-ord.dip orders/d4rus-ord.dip

Diagram 5:
./diplomacy configs/d5config.dip orders/d5ger-ord.dip

Diagram 6:
./diplomacy configs/d6config.dip orders/d6ord.dip

Diagram 7:
./diplomacy configs/d7config.dip orders/d7fra-ord.dip orders/d7eng-ord.dip

Diagram 7, plus an additional German Army in Kiel attacking Holland:
./diplomacy configs/d7config.dip orders/d7fra-ord.dip orders/d7eng-ord.dip orders/d7ger-ord.dip

Diagram 8:
./diplomacy configs/d8config.dip orders/d8ord.dip

Diagram 9:
./diplomacy configs/d9config.dip orders/d9ord.dip

Diagram 10:
./diplomacy configs/d10config.dip orders/d10ord.dip

Diagram 11:
./diplomacy configs/d11config.dip orders/d11ord.dip

Diagram 12:
./diplomacy configs/d12config.dip orders/d12ord.dip

Diagram 15:
./diplomacy configs/d15config.dip orders/d15ger-ord.dip orders/d15rus-ord.dip

Diagram 16:
./diplomacy configs/d15config.dip orders/d15ger-ord.dip orders/d16rus-ord.dip