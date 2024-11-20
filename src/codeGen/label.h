#pragma once

struct Labeller{
    size_t number = 0;
    
    size_t label(){
        return number++;
    }

};