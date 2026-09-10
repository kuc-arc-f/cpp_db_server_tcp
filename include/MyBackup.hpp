#pragma once
#include <iostream>
#include <vector>
#include "models.hpp"

class MyBackup {
private:
    std::string m_name;
    std::vector<QueItem> VecQue;

public:
    explicit MyBackup(std::string str){}
    ~MyBackup() {}

    int que_add(QueItem item, std::vector<QueItem> vec){
      vec.push_back(item);
      return 1;
    }

    /*
    QueItem que_get(){
      QueItem ret;
      if(VecQue.size() > 0){
        ret = VecQue[0];
      }
      return ret;
    }
    */

    int que_delete(QueItem item, std::vector<QueItem> vec){
      //VecQue.push_back(item);
      return 1;
    }    
};
