@author wenhm
@date 2014/10/8

KIT关键词过滤调整关键词,新增关键字的步骤：
1.修改prototype\src\common\keyword_finder\RestrictList.dat
2.编译prototype\src\common\keyword_finder\test_keyword_finder\test_keyword_finder.sln解决方案。
3.将生成的test_keyword_finder.exe文件拷贝到prototype\src\common\keyword_finder目录下执行即可。
4.新生成的prototype\src\common\keyword_finder\RestrictList.dat.dump即为新的关键词dump文件。
5.最后重新编译prototype\src\client\Utility工程即可。