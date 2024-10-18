#pragma once

#include <string>
#include <cstdlib>

#ifdef __ANDROID__
  inline std::string get_temp_dir(){
    return "/storage/emulated/0/Android/data/com.DefaultCompany.ailabdemo/cache/";
  }
#elif defined(__APPLE__) && defined(__MACH__)
    /* Apple OSX and iOS (Darwin). */
  #include <TargetConditionals.h>
  #if TARGET_OS_IPHONE == 1
      /* iOS */
    inline std::string get_temp_dir(){
      return std::string(getenv("HOME")) + "/tmp";
    }
  #else
    inline std::string get_temp_dir(){
      return "/tmp";
    }
  #endif
#else
    inline std::string get_temp_dir(){
      return "/tmp";
    }
#endif

inline std::string _append_path(const std::string &prefix, const std::string &post) {
  if (prefix.empty()) {
    return post;
  }
  if (post.empty()) {
    return prefix;
  }

  // 检查 prefix 的结尾是否有分隔符
  char last_char = prefix.back();
  if (last_char != '/') {
    // 如果没有分隔符，检查 post 的开头是否有分隔符
    if (post.front() != '/') {
      return prefix + '/' + post; // 两边都没有分隔符，添加一个分隔符
    }
  } else {
    // 如果 prefix 有分隔符，检查 post 的开头是否有分隔符
    if (post.front() == '/') {
      return prefix + post.substr(1); // 双重分隔符，去掉 post 开头的一个
    }
  }

  // 其他情况，直接连接
  return prefix + post;
}
inline std::string _get_filename(const std::string &p) {
  // 找到最后一个斜杠的位置
#if defined(_WIN32) || defined(_WIN64)
  std::size_t lastSlashPos = p.find_last_of("\\");
#else
  std::size_t lastSlashPos = p.find_last_of("/");
#endif
  // 如果找到斜杠，则文件名从斜杠之后开始，否则整个路径就是文件名
  if (lastSlashPos != std::string::npos) {
    return p.substr(lastSlashPos + 1);
  }
  return p;
}

// UTF-8 转 GBK 函数
inline std::string Utf8ToGbk(const std::string &utf8_str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wstr, len);
    
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* gbk_str = new char[len];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, gbk_str, len, NULL, NULL);
    
    std::string result(gbk_str);
    delete[] wstr;
    delete[] gbk_str;
    
    return result;
}