// // unique_ptr cannot be passed as a parameter. Use reference instead;
// // 函数 printInitList() 递归地打印出数组的初始值. 
// // Note: this is all wrong!!!!!
// void printInitVals_old(BaseASTPtr &initVal, vector<int32_t> offset){
//     if(!initVal){
//         std::cout << "zeroinit";
//         return;
//     }
//     int32_t arraySize = offset.back();
//     offset.pop_back();
//     if(!offset.size()){
//         std::cout << "{";
//         int32_t i;
//         for(i = 0; i < initVal->initVals.size(); i++){
//             if(i != arraySize-1)
//                 std::cout << initVal->initVals[i]->eval() << ","
//             else std::cout << initVal->initVals[i]->eval();
//         }
//         for(int32_t j = i; j < arraySize; j++){
//             if(j != arraySize-1)
//                 std::cout << "0" << ",";
//             else std::cout << "0";
//         }
//         std::cout<<"}";
//         return;
//     }
//     else{
//         std::cout<<"{";
//         int32_t subArraySize = offset.back(), curArraySize = 0;
//         InitValAST *newInitVal = new InitValAST();
//         for(int32_t i = 0; i < initVal->initVals.size(); i++){
//             if(!initVal->initVals[i] 
//             || initVal->initVals[i]->TypeOfAST() == "initVal"){
//                 int32_t addArraySize = getArraySize(offset,curArraySize);
//                 curArraySize += addArraySize;
//                 if(addArraySize == subArraySize){
//                     printInitVals(initVal->initVals[i],offset);
//                     if(curArraySize != arraySize)
//                         std::cout<<",";
//                     continue;
//                 }
//                 newInitVal->initVals.push_back(initVal->initVals[i]);
//                 if(curArraySize % subArraySize) 
//                     continue;
//                 printInitVals(newInitVal, offset);
//                 newInitVal->initVals.clear();
//                 if(curArraySize != arraySize)
//                     std::cout << ",";
//             }
//             else{
//                 newInitVal->initVals.push_back(initVal->initVals[i]);
//                 curArraySize++;
//                 if(curArraySize % subArraySize) 
//                     continue;
//                 printInitVals(newInitVal,offset);
//                 newInitVal->initVals.clear();
//                 if(curArraySize != arraySize)
//                     std::cout << ",";
//            }
//         }
//         if(newInitVal->initVals.size()){
//             printInitVals(newInitVal, offset);
//             // curArraySize += subArraySize;
//             curArraySize = (curArraySize / subArraySize + 1) * subArraySize;
//             if(curArraySize != arraySize);
//                 std::cout << ",";
//         }
//         while(curArraySize < arraySize){
//             curArraySize += subArraySize;
//             if(curArraySize != arraySize)
//                 std::cout << "zeroinit" << ",";
//             else std::cout << "zeroinit";
//         }
//         std::cout<<"}";
//         delete newInitVal;
//     }
// }
