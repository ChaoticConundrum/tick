/**
 * API data is taken from IEX Trading and Alpha Vantage
 * https://iextrading.com/developer/docs/
 * https://www.alphavantage.co/documentation/
 */

#ifndef IEX_H
#define IEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stddef.h>

struct string {
    char* data;
    size_t len;
};

typedef struct string String;

/**
 * Creates and returns a STRING
 * object with size 1 and no data
 * @return STRING object
 */
String* api_string_init(void);

/**
 * GETs data from API server and returns it in a String
 * @param url API url to GET
 * @return NULL if no response from server. Otherwise, String containing data.
 */
String* api_curl_data(char* url);

/**
 * Returns current price of a stock
 * @param ticker_name_string symbol
 * @return current price of stock
 */
double api_get_current_price(char* ticker_name_string);

/**
 * writefunction for cURL HTTP GET
 * stolen from a nice man on stackoverflow
 */
size_t api_string_writefunc(void* ptr, size_t size, size_t nmemb, String* hString);

/**
 * Returns current price of a stock with data from IEX.
 * Tested for NASDAQ, NYSE, and NYSEARCA listed stocks/ETFs.
 * Fast -- should take less than one second per call
 * @param ticker_name_string symbol
 * @return current price of stock
 */
double iex_get_current_price(char* ticker_name_string);

/**
 * Returns current price of a stock.
 * If the symbol is not on IEX, Alpha Vantage will be used
 * Tested for MUTF and OTCMKTS listed securities.
 * Dreadfully slow -- may take up to ten seconds per call
 * @param ticker_name_string symbol
 * @return current price of stock
 */
double alphavantage_get_current_price(char* ticker_name_string);

/**
 * Destroys STRING object and frees memory
 * @param phString the String to destroy
 */
void api_string_destroy(String** phString);

#endif