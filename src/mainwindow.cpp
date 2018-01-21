#include "mainwindow.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <map>
#include <utility>
#include <algorithm>
#include <iterator>
#include <ctime>

#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

using namespace rapidjson;

mainwindow::mainwindow() : Fl_Window(420, 430, "MiningSpeedChecker")
{
  taddInput = new Fl_Input(5, 0, 300, 30, "Enter taddress:");
  refreshButton = new Fl_Button(311, 0, 99, 30, "Refresh");
  minerBox = new Fl_Box(10, 30, 400, 30, " ");
  totalSolsBox = new Fl_Box(10, 60, 400, 30, " ");
  networkSolsBox = new Fl_Box(10, 90, 400, 30, " ");
  immaturebalanceBox = new Fl_Box(10, 120, 400, 30, " ");
  balanceBox = new Fl_Box(10, 150, 400, 30, " ");
  paidBox = new Fl_Box(10, 180, 400, 30, " ");
  hashrateChart = new Fl_Chart(10, 210, 400, 200, "Chart");
  hashrateChart->type(FL_LINE_CHART);
  refreshButton->callback(onRefreshButtonPressed, (void*)this);
  checkRead();
}

mainwindow::~mainwindow() {}

void mainwindow::onRefreshButtonPressed(Fl_Widget* w, void* data)
{
  mainwindow *o = (mainwindow*)data;
  o->update(o->taddInput->value());
}

void mainwindow::checkRead()
{
  std::ifstream f("config.json");
  if (f.good())
  {
    std::string contents = "", line;
    while(!f.eof())
    {
      getline(f, line);
      contents += line;
    }
    Document d;
    d.Parse(contents.c_str());
    if (d.HasMember("taddress"))
    {
      assert(d["taddress"].IsString());
      std::string tadd = d["taddress"].GetString();
      taddInput->value(tadd.c_str());
      conf_exists = true;
      update(tadd);
    }
  }
}

/* User by mainwindow::getFromAPI to put websource into a string type */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}
/* Uses LibCurl to get Data from API */
void mainwindow::update(std::string tadd)
{
  CURL *curl;
  CURLcode res;
  std::string readBuffer;
  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, (address + "/api/worker_stats?" + tadd).c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
#ifndef NDEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
    {
      fl_alert("curl_easy_perform() failed: %s",
        curl_easy_strerror(res));
      return;
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
    //--------------------------------------
#ifndef NDEBUG
    std::cout << readBuffer << std::endl;
#endif
    Document d;
    d.Parse(readBuffer.c_str());
    std::string totalSols, networkSols, immaturebalance, balance, paid;
    bool h_0 = d.HasMember("totalHash");
    bool h_1 = d.HasMember("networkSols");
    bool h_2 = d.HasMember("immature");
    bool h_3 = d.HasMember("balance");
    bool h_4 = d.HasMember("paid");
    bool h_5 = d.HasMember("pool");
    if (!(h_0 & h_1 & h_2 & h_3 & h_4 & h_5))
    {
      fl_alert("Error: returned from API is missing information.");
      return;
    }
    if(d["networkSols"].IsInt())
    {
      fl_alert("Error: account is not active.");
      return;
    }

    std::string pool = (d["pool"].IsString()) ? d["pool"].GetString() : "";

    std::ostringstream buffer;
    buffer << "Total Hashrate: " << std::setprecision(8) << (d["totalHash"].GetDouble() / 500000.0) << " Sol/s";
    totalSols = buffer.str();

    std::ostringstream buffer1;
    buffer1 << "Network Hashrate: " << std::setprecision(8) << 
              (std::stod(d["networkSols"].GetString()) / 1000000) << " MSol/s"; 
    networkSols = buffer1.str();

    std::ostringstream buffer2;
    buffer2 << "Immature: " << std::setprecision(8) << d["immature"].GetDouble() << " " << pool; 
    immaturebalance = buffer2.str();

    std::ostringstream buffer3;
    buffer3 << "Balance: " <<  std::setprecision(8) << d["balance"].GetDouble() << " " << pool;
    balance = buffer3.str();

    std::ostringstream buffer4;
    buffer4 << "Paid: " << std::setprecision(8) << d["paid"].GetDouble() << " " << pool;
    paid = buffer4.str();

    auto conv_fun = [](std::time_t t, double hr) {
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&t), "%I:%M%p");
        ss << std::setprecision(2) << hr << "H/s";
        return ss.str();
      };

    std::map<std::time_t, double> info;
    const Value& a = d["history"];
    bool first = true;
    int _min, _max;
    for(Value::ConstMemberIterator itr = a.MemberBegin();
        itr != a.MemberEnd(); ++itr)
    {
      const Value& b = itr->value;
      assert(b.IsArray());
      for (SizeType i = 0; i < b.Size(); i++)
      {
        const Value& las = b[i];
        double hashrate = las["hashrate"].GetDouble() / 500000.0;
        std::time_t time = static_cast<std::time_t>(las["time"].GetInt());
        if (!info.count(time))
          info.insert(std::pair<std::time_t, double>(time, hashrate));
        else
          info.at(time)+=hashrate;

        if (first)
        {
          _min = _max = hashrate;
          first = false;
        }
        else
        {
          if (_min > hashrate)
            _min = hashrate;
          if (_max < hashrate)
            _max = hashrate;
        }
      }
    }

    hashrateChart->clear();
    hashrateChart->bounds(_min, _max);
    int i = 0;
    for(auto& item : info) {
      if (i % 2 == 0)
        hashrateChart->add(item.second, 
        (i % 30 == 0 && i != 0) ? conv_fun(item.first, item.second).c_str() : 0, 
        FL_RED);
      i++;
    }
    hashrateChart->redraw();

    if(!conf_exists || minerBox->label() != taddInput->value())
      create_or_modify_config(tadd);

    minerBox->copy_label(tadd.c_str());
    totalSolsBox->copy_label(totalSols.c_str());
    networkSolsBox->copy_label(networkSols.c_str());
    immaturebalanceBox->copy_label(immaturebalance.c_str());
    balanceBox->copy_label(balance.c_str());
    paidBox->copy_label(paid.c_str());
  }
}
void mainwindow::create_or_modify_config(std::string tadd)
{
  Document d;
  d.SetObject();
  Document::AllocatorType& allocator = d.GetAllocator();

  Value v_tadd;
  v_tadd = StringRef(tadd.c_str());
  d.AddMember("taddress", v_tadd, allocator);

  StringBuffer strbuf;
  PrettyWriter<StringBuffer> writer(strbuf);
  d.Accept(writer);
  std::string json = strbuf.GetString();

  std::fstream file;
  file.open("config.json", std::ios::trunc | std::ios::out);
  file << json;
  file.close();
  conf_exists = true;
}
