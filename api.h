/**
 * API data is taken from IEX Trading, Morningstar, Coinmarketcap, and News API.
 * https://iextrading.com/developer/docs/
 * http://www.morningstar.com/
 * https://coinmarketcap.com/api/
 * https://newsapi.org/docs
 */

#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <curl/curl.h>
#include <json-c/json_tokener.h>

struct string {
    char* data;
    size_t len;
};

typedef struct string String;

typedef struct json_object Json;

/**
 * Creates and returns a STRING
 * object with size 1 and no data
 * @return STRING object
 */
String* api_string_init(void);

/**
 * GETs data from API server and returns it in a String
 * @param url API url to GET
 * @param post_field data needed for POST
 * @return NULL if no response from server. Otherwise, String containing data.
 */
String* api_curl_data(char* url, char* post_field);

/**
 * Returns a double* containing the current price and yesterday's price of a stock or cryptocurrency.
 * [0] = current intraday price
 * [1] = last close price
 * Order:
 * 1. IEX -- NASDAQ/NYSE/NYSEARCA
 * 2. Morningstar -- MUTF/OTCMKTS
 * 3. Coinmarketcap -- CRYPTO
 * @param ticker_name_string symbol
 * @return price data
 */
double* api_get_current_price(char* ticker_name_string);

/**
 * writefunction for cURL HTTP GET
 * stolen from a nice man on stackoverflow
 */
size_t api_string_writefunc(void* ptr, size_t size, size_t nmemb, String* hString);

/**
 * Returns current and yesterday's price of a stock with data from IEX.
 * Tested for NASDAQ, NYSE, and NYSEARCA listed stocks/ETFs.
 * @param ticker_name_string symbol
 * @return current price of stock
 */
double* iex_get_price(char* ticker_name_string);

/**
 * Returns current and yesterday's price of a mutual fund with data from Morningstar
 * Tested for MUTF and OTCMKTS listed securities.
 * @param ticker_name_string symbol
 * @param offset number of days ago to get price of (0 = today)
 * @return price of security
 */
double* morningstar_get_price(char* ticker_name_string);

/**
 * Returns current and yesterday's price of a cryptocurrency with data from Coinmarketcap.
 * All cryptocurrencies listed on Coinmarketcap will work.
 * @param ticker_name_string symbol
 * @return current price of cryptocurrency
 */
double* coinmarketcap_get_price(char* ticker_name_string);

/**
 * Prints top three news articles in the past week based on the given string
 * @param ticker_name_string the string
 */
void news_print_top_three(char* ticker_name_string);

/**
 * Given a JSON formatted string, print title, source, author, and url of articles
 * @param data the json formatted data
 */
void json_print_news(Json* jobj);

/**
 * Given a url, returns a shorter link using goo.gl
 * @param url_string the link to shorten
 * @return the shortened link
 */
const char* google_shorten_link(char* url_string);

/**
 * Returns a string stripped of the given char
 * @param string the string to strip the char from
 * @param c the char to strip
 * @return the stripped string
 */
const char* strip_char(char* string, char c);

/**
 * Destroys String object and frees memory
 * @param phString the String to destroy
 */
void api_string_destroy(String** phString);

#endif