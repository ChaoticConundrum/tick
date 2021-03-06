#include "api.h"

String* api_string_init() {
    String* pString = (String*) malloc(sizeof(String));
    if (pString != NULL) {
        pString->len = 0;
        pString->data = (char*) malloc(sizeof(char));
        if (pString->data == NULL) {
            fprintf(stderr, "malloc() failed\n");
            exit(EXIT_FAILURE);
        }
        pString->data[0] = '\0';
    } else {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    return pString;
}

size_t api_string_writefunc(void* ptr, size_t size, size_t nmemb, String* hString) {
    String* pString = hString;
    size_t new_len = pString->len + size * nmemb;
    pString->data = realloc(pString->data, new_len + 1);
    if (pString->data == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(pString->data + pString->len, ptr, size * nmemb);
    pString->data[new_len] = '\0';
    pString->len = new_len;
    return size * nmemb;
}

String* api_curl_data(char* url, char* post_field) {
    String* pString = api_string_init();
    CURL* curl = curl_easy_init();
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_string_writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pString->data);
        struct curl_slist* list = NULL;
        if (url[12] == 'g') { //if using Google Urlshortener, a post field is needed
            list = curl_slist_append(list, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_field);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_field));
        }
        res = curl_easy_perform(curl);
        if (url[12] == 'g')
            curl_slist_free_all(list);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK)
            return NULL;
    }
    return pString;
}

double* api_get_current_price(char* ticker_name_string) {
    double* val;
    if (strlen(ticker_name_string) > 5) { //if symbol length is greater than 5, then it must be a crypto
        val = coinmarketcap_get_price(ticker_name_string);
        return val;
    }
    val = iex_get_price(ticker_name_string); //First tries IEX for intraday prices
    if (val != NULL)
        return val;
    val = morningstar_get_price(ticker_name_string); //Secondly tries Morningstar for market open/close data
    if (val != NULL)
        return val;
    val = coinmarketcap_get_price(ticker_name_string); //Thirdly tries Coinmarketcap for real-time crypto data
    if (val != NULL)
        return val;
    return NULL; //Invalid symbol
}

double* iex_get_price(char* ticker_name_string) {
    char* iex_api_string = calloc(64, sizeof(char));
    sprintf(iex_api_string, "%s%s%s", "https://api.iextrading.com/1.0/stock/", ticker_name_string, "/quote");
    String* pString = api_curl_data(iex_api_string, NULL);
    free(iex_api_string);
    if (strcmp(pString->data, "Unknown symbol") == 0) { //Invalid symbol
        api_string_destroy(&pString);
        return NULL;
    }
    Json* jobj = json_tokener_parse(pString->data);
    double* ret = malloc(sizeof(double) * 2);
    char* price_string = (char*) json_object_to_json_string(json_object_object_get(jobj, "latestPrice"));
    char* close_price_string = (char*) json_object_to_json_string(json_object_object_get(jobj, "previousClose"));
    ret[0] = strtod(price_string, NULL); //Intraday current price
    ret[1] = strtod(close_price_string, NULL); //Previous day's close price
    api_string_destroy(&pString);
    json_object_put(jobj);
    return ret;
}

double* morningstar_get_price(char* ticker_name_string) {
    time_t now = time(NULL);
    struct tm* ts;
    char* today_char = calloc(16, 1);
    char* yesterday_char = calloc(16, 1);
    ts = localtime(&now);
    mktime(ts);
    strftime(today_char, 16, "%Y-%m-%d", ts);
    ts->tm_mday -= 7; //get info from past 7 days
    mktime(ts);
    strftime(yesterday_char, 16, "%Y-%m-%d", ts);
    char* morningstar_api_string = calloc(512, 1);
    sprintf(morningstar_api_string, "%s%s%s%s|%s",
            "http://globalquote.morningstar.com/globalcomponent/RealtimeHistoricalStockData.ashx?ticker=",
            ticker_name_string,
            "&showVol=true&dtype=his&f=d&curry=USD&isD=true&isS=true&hasF=true&ProdCode=DIRECT&range=", yesterday_char,
            today_char);
    String* pString = api_curl_data(morningstar_api_string, NULL);
    free(morningstar_api_string);
    free(today_char);
    free(yesterday_char);
    Json* jobj = json_tokener_parse(pString->data);
    if (strcmp("null", pString->data) == 0) { //Invalid symbol
        api_string_destroy(&pString);
        return NULL;
    }
    double* ret = malloc(sizeof(double) * 2);
    Json* datapoints = json_object_object_get(
            json_object_array_get_idx(json_object_object_get(jobj, "PriceDataList"), 0), "Datapoints");
    size_t size = json_object_array_length(datapoints);
    Json* today = json_object_array_get_idx(json_object_array_get_idx(datapoints, size - 1), 0); //latest day of data will be in last array index
    char* price = (char*) json_object_to_json_string(today);
    ret[0] = strtod(price, NULL); //Last close price
    Json* yesterday = json_object_array_get_idx(json_object_array_get_idx(datapoints, size - 2), 0);
    price = (char*) json_object_to_json_string(yesterday);
    ret[1] = strtod(price, NULL); //Close price before last close price
    json_object_put(jobj);
    api_string_destroy(&pString);
    return ret;
}

double* coinmarketcap_get_price(char* ticker_name_string) {
    char* coinmarketcap_api_string = calloc(64, sizeof(char));
    sprintf(coinmarketcap_api_string, "%s%s", "https://api.coinmarketcap.com/v1/ticker/", ticker_name_string);
    String* pString = api_curl_data(coinmarketcap_api_string, NULL);
    free(coinmarketcap_api_string);
    if (pString->data[0] == '{') { //Invalid symbol
        api_string_destroy(&pString);
        return NULL;
    }
    Json* jobj = json_tokener_parse(pString->data);
    Json* data = json_object_array_get_idx(jobj, 0);
    Json* usd = json_object_object_get(data, "price_usd");
    Json* percent_change_1d = json_object_object_get(data, "percent_change_24h");
    char* price = (char*) json_object_get_string(usd);
    char* change_1d = (char*) json_object_get_string(percent_change_1d);
    double* ret = malloc(sizeof(double) * 2);
    ret[0] = strtod(price, NULL); //Current real-time price
    ret[1] = ret[0] - ((strtod(change_1d, NULL) / 100) * ret[0]); //Price 24 hours earlier
    api_string_destroy(&pString);
    json_object_put(jobj);
    return ret;
}

void news_print_top_three(char* ticker_name_string) {
    char* url_encoded_string = calloc(128, 1);
    for (int i = 0, j = 0; i < 128; i++, j++) { //Replace underscores and spaces with url encoded '%20's
        if (ticker_name_string[i] == '_' || ticker_name_string[i] == ' ') {
            memcpy(&url_encoded_string[i], "%20", 3);
            i += 2;
        } else url_encoded_string[i] = ticker_name_string[j];
    }
    time_t now = time(NULL);
    struct tm* ts;
    char* yearchar = calloc(64, 1);
    ts = localtime(&now);
    ts->tm_mday -= 14; // Lowerbound date is 14 days earlier
    mktime(ts);
    strftime(yearchar, 64, "%Y-%m-%d", ts);
    char* news_api_string = calloc(256, sizeof(char));
    sprintf(news_api_string, "%s&from=%s&q=%s",
            "https://newsapi.org/v2/everything?sortBy=relevancy&pageSize=3&language=en&apiKey=1163c352d041460381f0a8273e60a9d1",
            yearchar, url_encoded_string);
    free(yearchar);
    free(url_encoded_string);
    String* pString = api_curl_data(news_api_string, NULL);
    Json* jobj = json_tokener_parse(pString->data);
    if (strtol(json_object_to_json_string(json_object_object_get(jobj, "totalResults")), NULL,
               10) > 0)
        json_print_news(jobj);
    else printf("No articles. Try a different input.\n");
    free(news_api_string);
    api_string_destroy(&pString);
    json_object_put(jobj);
}

void json_print_news(Json* jobj) {
    Json* article_list = json_object_object_get(jobj, "articles");
    Json* article;
    char* author_string, * title_string, * source_string, * date_string, * url_string;
    int results = (int) strtol(json_object_to_json_string(json_object_object_get(jobj, "totalResults")), NULL,
                               10);
    for (int i = 0; i < results && i < 3; i++) {
        article = json_object_array_get_idx(article_list, (size_t) i);
        author_string = (char*) strip_char(
                (char*) json_object_to_json_string(json_object_object_get(article, "author")),
                '\\'); //Strip all attributes of backslashes
        title_string = (char*) strip_char(
                (char*) json_object_to_json_string(json_object_object_get(article, "title")), '\\');
        source_string = (char*) strip_char((char*) json_object_to_json_string(
                json_object_object_get(json_object_object_get(article, "source"), "name")), '\\');
        date_string = (char*) json_object_to_json_string(json_object_object_get(article, "publishedAt"));
        url_string = (char*) strip_char((char*) json_object_to_json_string(json_object_object_get(article, "url")),
                                        '\\');
        char* stripped = (char*) strip_char(url_string, '\"'); //Strip url of quotes
        char* shortened = (char*) google_shorten_link(stripped); //Shorten link
        printf("Title: %s Source: %s ", title_string, source_string);
        if (strcmp(author_string, "null") != 0) //Some articles don't list authors or dates
            printf("Author: %s ", author_string);
        if (strcmp(date_string, "null") != 0) {
            date_string[11] = '\"';
            date_string[12] = '\0'; //Terminate date string after the day of the month since it include time as well
            printf("Date: %s ", date_string);
        }
        printf("Url: \"%s\"\n", shortened);
        free(author_string);
        free(title_string);
        free(source_string);
        free(url_string);
        free(stripped);
        free(shortened);
    }
}

const char* google_shorten_link(char* url_string) {
    char* google_api_string = "https://www.googleapis.com/urlshortener/v1/url?key=AIzaSyAoMAvMPpc7U8lfrnGMk2ZKl966tU2pppU";
    char* post_string = calloc(1024, 1);
    sprintf(post_string, "{\"longUrl\": \"%s\"}", url_string); //Format HTTP POST
    String* pString = api_curl_data(google_api_string, post_string);
    free(post_string);
    Json* jobj = json_tokener_parse(pString->data);
    Json* short_url = json_object_object_get(jobj, "id");
    const char* short_url_string = json_object_to_json_string(short_url);
    char* copy = calloc(1024, 1);
    strcpy(copy, short_url_string);
    char* final = calloc(strlen(copy) + 1, 1);
    for (int i = 0, j = 0; j < strlen(copy); i++, j++) { //Ignore escape characters
        if (copy[j] == '\\' || copy[j] == '\"')
            j++;
        final[i] = copy[j];
    }
    json_object_put(jobj);
    free(copy);
    api_string_destroy(&pString);
    return final;
}

const char* strip_char(char* string, char c) {
    size_t len = strlen(string);
    char* final_string = calloc(len + 1, 1);
    for (int i = 0, j = 0; j < (int) len; i++, j++) {
        while (string[j] == c)
            j++;
        final_string[i] = string[j];
    }
    return final_string;
}

void api_string_destroy(String** phString) {
    String* pString = *phString;
    free(pString->data);
    free(*phString);
    *phString = NULL;
}