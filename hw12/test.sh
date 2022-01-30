#!/bin/bash

diff >/dev/null <(./main test moscow) <(echo '                        
##### #####  #### ##### 
  #   #     #       #   
  #   ####  #       #   
  #   #     #       #   
  #   #####  ####   #   ') || (echo 'test failed' ; exit 1)
