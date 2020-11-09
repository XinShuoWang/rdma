//
// Created by wangxinshuo on 2020/11/9.
//

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

class TcpClient{
 private:

 public:

};

using namespace std;
using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
  try {
    //(1)通过tcp::socket类定义一个tcp client对象socket
    boost::asio::io_service io;
    tcp::socket socket(io);
    //(2)通过connect函数连接服务器，打开socket连接。
    tcp::endpoint end_point(boost::asio::ip::address::from_string("127.0.0.1"), 8080);
    socket.connect(end_point);
    for (;;) {
      boost::array<char, 128> buf{};
      boost::system::error_code error;
      //(3)通过read_some函数来读数据
      size_t len = socket.read_some(boost::asio::buffer(buf), error);
      if (error == boost::asio::error::eof) {
        break;    //connection closed cleadly by peer
      } else if (error) {
        throw boost::system::system_error(error);    //some other error
      }
      cout.write(buf.data(), len);
    }
  }
  catch (std::exception &e) {
    cout << e.what() << endl;
  }
}