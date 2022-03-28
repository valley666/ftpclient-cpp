#include "test_utils.h"

// Test configuration constants (to be loaded from an INI file)
bool FTP_TEST_ENABLED;
bool SFTP_TEST_ENABLED;
bool HTTP_PROXY_TEST_ENABLED;

std::string CURL_LOG_FOLDER;

std::string SSL_CERT_FILE;
std::string SSL_KEY_FILE;
std::string SSL_KEY_PWD;

std::string FTP_SERVER;
unsigned FTP_SERVER_PORT;
std::string FTP_USERNAME;
std::string FTP_PASSWORD;
std::string FTP_REMOTE_FILE;
std::string FTP_REMOTE_FILE_SHA1SUM;
std::string FTP_REMOTE_UPLOAD_FOLDER;
std::string FTP_REMOTE_DOWNLOAD_FOLDER;

std::string SFTP_SERVER;
unsigned SFTP_SERVER_PORT;
std::string SFTP_USERNAME;
std::string SFTP_PASSWORD;
std::string SFTP_REMOTE_FILE;
std::string SFTP_REMOTE_FILE_SHA1SUM;
std::string SFTP_REMOTE_UPLOAD_FOLDER;
std::string SFTP_REMOTE_DOWNLOAD_FOLDER;

std::string PROXY_SERVER;
std::string PROXY_SERVER_FAKE;

std::mutex g_mtxConsoleMutex;

bool GlobalTestInit() {
   const std::string& strConfFile = "./TestFTP/template_test_conf.ini";

   CSimpleIniA ini;
   if(SI_OK != ini.LoadFile(strConfFile.c_str()))
      return false;

   std::string strTmp;
   strTmp = ini.GetValue("tests", "ftp", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   FTP_TEST_ENABLED = (strTmp == "YES") ? true : false;

   strTmp = ini.GetValue("tests", "sftp", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   SFTP_TEST_ENABLED = (strTmp == "YES") ? true : false;

   strTmp = ini.GetValue("tests", "http-proxy", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   HTTP_PROXY_TEST_ENABLED = (strTmp == "YES") ? true : false;

   // required when a build is generated with the macro DEBUG_CURL
   CURL_LOG_FOLDER = ini.GetValue("local", "curl_logs_folder", "");

   SSL_CERT_FILE = ini.GetValue("local", "ssl_cert_file", "");
   SSL_KEY_FILE  = ini.GetValue("local", "ssl_key_file", "");
   SSL_KEY_PWD   = ini.GetValue("local", "ssl_key_pwd", "");

   PROXY_SERVER      = ini.GetValue("http-proxy", "host", "");
   PROXY_SERVER_FAKE = ini.GetValue("http-proxy", "host_invalid", "");

   FTP_SERVER                 = ini.GetValue("ftp", "host", "");
   FTP_SERVER_PORT            = atoi(ini.GetValue("ftp", "port", "0"));
   FTP_USERNAME               = ini.GetValue("ftp", "username", "");
   FTP_PASSWORD               = ini.GetValue("ftp", "password", "");
   FTP_REMOTE_FILE            = ini.GetValue("ftp", "remote_file", "");
   FTP_REMOTE_FILE_SHA1SUM    = ini.GetValue("ftp", "remote_file_sha1sum", "");
   FTP_REMOTE_UPLOAD_FOLDER   = ini.GetValue("ftp", "remote_upload_folder", "");
   FTP_REMOTE_DOWNLOAD_FOLDER = ini.GetValue("ftp", "remote_download_folder", "");

   SFTP_SERVER                 = ini.GetValue("sftp", "host", "");
   SFTP_SERVER_PORT            = atoi(ini.GetValue("sftp", "port", "0"));
   SFTP_USERNAME               = ini.GetValue("sftp", "username", "");
   SFTP_PASSWORD               = ini.GetValue("sftp", "password", "");
   if(SFTP_PASSWORD.empty())   SFTP_PASSWORD = ' ';
   SFTP_REMOTE_FILE            = ini.GetValue("sftp", "remote_file", "");
   SFTP_REMOTE_FILE_SHA1SUM    = ini.GetValue("sftp", "remote_file_sha1sum", "");
   SFTP_REMOTE_UPLOAD_FOLDER   = ini.GetValue("sftp", "remote_upload_folder", "");
   SFTP_REMOTE_DOWNLOAD_FOLDER = ini.GetValue("sftp", "remote_download_folder", "");

   if (!FTP_REMOTE_UPLOAD_FOLDER.empty() && FTP_REMOTE_UPLOAD_FOLDER.at(FTP_REMOTE_UPLOAD_FOLDER.length() - 1) != '/') {
      FTP_REMOTE_UPLOAD_FOLDER += '/';
   }

   if (!FTP_REMOTE_DOWNLOAD_FOLDER.empty()) {
      if (FTP_REMOTE_DOWNLOAD_FOLDER.at(FTP_REMOTE_DOWNLOAD_FOLDER.length() - 1) != '/')
         FTP_REMOTE_DOWNLOAD_FOLDER += "/*";
      else
         FTP_REMOTE_DOWNLOAD_FOLDER += "*";
   }

   if (!SFTP_REMOTE_UPLOAD_FOLDER.empty() && SFTP_REMOTE_UPLOAD_FOLDER.at(SFTP_REMOTE_UPLOAD_FOLDER.length() - 1) != '/') {
      SFTP_REMOTE_UPLOAD_FOLDER += '/';
   }

   if (!SFTP_REMOTE_DOWNLOAD_FOLDER.empty()) {
      if (SFTP_REMOTE_DOWNLOAD_FOLDER.at(SFTP_REMOTE_DOWNLOAD_FOLDER.length() - 1) != '/')
         SFTP_REMOTE_DOWNLOAD_FOLDER += "/*";
      else
         SFTP_REMOTE_DOWNLOAD_FOLDER += "*";
   }

   if ((FTP_TEST_ENABLED && (FTP_SERVER.empty() || FTP_SERVER_PORT == 0)) ||
       (SFTP_TEST_ENABLED && (SFTP_SERVER.empty() || SFTP_SERVER_PORT == 0)) ||
       (HTTP_PROXY_TEST_ENABLED && (PROXY_SERVER.empty() || PROXY_SERVER_FAKE.empty()))) {
      std::clog << "[ERROR] Check your INI file parameters."
                   " Disable tests that don't have a server/port value."
                << std::endl;
      return false;
   }

   return true;
}

void GlobalTestCleanUp(void) { return; }

void TimeStampTest(std::ostringstream& ssTimestamp) {
   time_t tRawTime;
   tm* tmTimeInfo;
   time(&tRawTime);
   tmTimeInfo = localtime(&tRawTime);

   ssTimestamp << (tmTimeInfo->tm_year) + 1900 << "/" << tmTimeInfo->tm_mon + 1 << "/" << tmTimeInfo->tm_mday << " at "
               << tmTimeInfo->tm_hour << ":" << tmTimeInfo->tm_min << ":" << tmTimeInfo->tm_sec;
}

// for uplaod prog. callback, just use DL one and inverse download parameters with upload ones...
int TestUPProgressCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded) {
   return TestDLProgressCallback(ptr, dTotalToUpload, dNowUploaded, dTotalToDownload, dNowDownloaded);
}
int TestDLProgressCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded) {
   // ensure that the file to be downloaded is not empty
   // because that would cause a division by zero error later on
   if (dTotalToDownload <= 0.0) return 0;

   // how wide you want the progress meter to be
   const int iTotalDots       = 20;
   double dFractionDownloaded = dNowDownloaded / dTotalToDownload;
   // part of the progressmeter that's already "full"
   int iDots = static_cast<int>(round(dFractionDownloaded * iTotalDots));

   // create the "meter"
   int iDot = 0;
   std::cout << static_cast<unsigned>(dFractionDownloaded * 100) << "% [";

   // part  that's full already
   for (; iDot < iDots; iDot++) std::cout << "=";

   // remaining part (spaces)
   for (; iDot < iTotalDots; iDot++) std::cout << " ";

   // and back to line begin - do not forget the fflush to avoid output buffering problems!
   std::cout << "]           \r" << std::flush;

   // if you don't return 0, the transfer will be aborted - see the documentation
   return 0;
}

long GetGMTOffset() {
   time_t now = time(nullptr);

   struct tm gm = *gmtime(&now);
   time_t gmt   = mktime(&gm);

   struct tm loc = *localtime(&now);
   time_t local  = mktime(&loc);

   return static_cast<long>(difftime(local, gmt));
}

bool GetFileTime(const char* const& pszFilePath, time_t& tLastModificationTime) {
   FILE* pFile = fopen(pszFilePath, "rb");

   if (pFile != nullptr) {
      struct stat file_info;

#ifndef LINUX
      if (fstat(_fileno(pFile), &file_info) == 0)
#else
      if (fstat(fileno(pFile), &file_info) == 0)
#endif
      {
         tLastModificationTime = file_info.st_mtime;
         return true;
      }
   }

   return false;
}

long long GetFileSize(const std::string& filename) {
   struct stat stat_buf;
   int rc = stat(filename.c_str(), &stat_buf);
   return rc == 0 ? stat_buf.st_size : -1;
}

long long FdGetFileSize(int fd) {
   struct stat stat_buf;
   int rc = fstat(fd, &stat_buf);
   return rc == 0 ? stat_buf.st_size : -1;
}

std::string sha1sum(const std::vector<char>& memData) {
   CSHA1 sha1;
   sha1.Update(reinterpret_cast<const unsigned char*>(memData.data()), memData.size());
   sha1.Final();

   std::string strHash;
   sha1.ReportHashStl(strHash, CSHA1::REPORT_HEX_SHORT);

   return strHash;
}

std::string sha1sum(const std::string& filename) {
   long long fileSize = GetFileSize(filename);
   if (fileSize <= 0) {
      return std::string();
   }

   std::ifstream file(filename, std::ios::binary);
   if (!file) {
      return std::string();
   }

   CSHA1 sha1;

   char* pFileBuffer;
   static const size_t s_bufferSize = 32 * 20 * 820;
   try {
      pFileBuffer = new char[s_bufferSize];
   } catch (...) {
      return std::string();
   }

   // size_t bytesProcessed = 0;
   while (true) {
      file.read(pFileBuffer, s_bufferSize);
      const size_t readBytes = file.gcount();

      if (readBytes > 0) {
         sha1.Update(reinterpret_cast<const unsigned char*>(pFileBuffer), readBytes);
      } else {
         break;
      }

      // bytesProcessed += readBytes;
   }
   delete[] pFileBuffer;

   // Finalize hash
   sha1.Final();

   std::string strHash;
   sha1.ReportHashStl(strHash, CSHA1::REPORT_HEX_SHORT);

   return strHash;
}
