#include "mainwindow.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <functional>

#include <curl/curl.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

using namespace rapidjson;

mainwindow::mainwindow() : Fl_Window(400, 211, "MiningSpeedChecker")
{
  taddInput = new Fl_Input(0, 0, 300, 30, "Enter taddress:");
  refreshButton = new Fl_Button(301, 0, 99, 30, "Refresh");
  minerBox = new Fl_Box(0, 30, 400, 30, " ");
  totalSolsBox = new Fl_Box(0, 60, 400, 30, " ");
  networkSolsBox = new Fl_Box(0, 90, 400, 30, " ");
  immaturebalanceBox = new Fl_Box(0, 120, 400, 30, " ");
  balanceBox = new Fl_Box(0, 150, 400, 30, " ");
  paidBox = new Fl_Box(0, 180, 400, 30, " ");
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
    Document d;
    d.Parse(readBuffer.c_str());
    std::string totalSols, networkSols, immaturebalance, balance, paid;
    bool h_0 = d.HasMember("totalHash");
    bool h_1 = d.HasMember("networkSols");
    bool h_2 = d.HasMember("immature");
    bool h_3 = d.HasMember("balance");
    bool h_4 = d.HasMember("paid");
    if (!(h_0 & h_1 & h_2 & h_3 & h_4))
    {
      fl_alert("Error: returned from API is missing information.");
      return;
    }

    assert(d["totalHash"].IsDouble());
    assert(d["networkSols"].IsString());
    assert(d["immature"].IsDouble());
    assert(d["balance"].IsDouble());
    assert(d["paid"].IsDouble());

    std::ostringstream buffer;
    buffer << "Total Hashrate: " << std::setprecision(8) << (d["totalHash"].GetDouble() / 500000.0) << " Sol/s";
    totalSols = buffer.str();

    std::ostringstream buffer1;
    buffer1 << "Network Hashrate: " << std::setprecision(8) << 
              (std::stod(d["networkSols"].GetString()) / 1000000) << " MSol/s"; 
    networkSols = buffer1.str();

    std::ostringstream buffer2;
    buffer2 << "Immature: " << std::setprecision(8) << d["immature"].GetDouble() << " Hush"; 
    immaturebalance = buffer2.str();

    std::ostringstream buffer3;
    buffer3 << "Balance: " <<  std::setprecision(8) << d["balance"].GetDouble() << " Hush";
    balance = buffer3.str();

    std::ostringstream buffer4;
    buffer4 << "Paid: " << std::setprecision(8) << d["paid"].GetDouble() << " Hush";
    paid = buffer4.str();

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
