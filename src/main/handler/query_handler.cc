#include "handler/query_handler.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "data/query_request.h"
#include "data/query_request_parser.h"
#include "data/query_result.h"
#include "query/query_executor.h"
#include "service/request_context.h"
#include "service/response_writer.h"
#include "utils/logger.h"
#include "utils/stop_watch.h"

namespace redgiant {

DECLARE_LOGGER(logger, __FILE__);

void QueryHandler::handle_request(const RequestContext* request, ResponseWriter* response) {
  StopWatch watch;

  int method = request->get_method();
  if (method != RequestContext::METHOD_POST) {
    response->add_body("method should be POST\n");
    response->send(400, NULL);
    LOG_ERROR(logger, "method is not POST");
    return;
  }

  int post_len = request->get_content_length();
  if (post_len <= 0) {
    response->add_body("content is missing\n");
    response->send(400, NULL);
    LOG_ERROR(logger, "content is missing");
    return;
  }

  std::string request_id = request->get_query_param("id");
  std::string ranking_model = request->get_query_param("model");
  std::string query_count_str = request->get_query_param("count");
  std::string debug = request->get_query_param("debug");

  int query_count = 10;
  if (!query_count_str.empty()) {
    query_count = atoi(query_count_str.c_str());
    if (query_count == 0) {
      response->add_body(R"({"ret":"param_error", "results":[]})""\n");
      response->send(400, NULL);
      LOG_INFO(logger, "[query:%s] error=param_error, latency=%ldus", request_id.c_str(), watch.get_ticks_us());
      return;
    }
  }

  QueryRequest query_request(request_id, query_count, ranking_model, watch, debug == "true");
  if (query_request.is_debug()) {
    LOG_INFO(logger, "[query:%s] model:%s, query_count:%d", request_id.c_str(), ranking_model.c_str(), query_count);
  }

  buf_.alloc(post_len + 1);
  char* body = buf_.data();
  int ret_len = request->get_content(body, post_len);
  body[ret_len] = 0;

  if (query_request.is_debug()) {
    LOG_TRACE(logger, "[query:%s] query request: uri=%s, post=%s", request_id.c_str(), request->get_uri().c_str(), body);
  }

  int parse_ret = parser_->parse(body, ret_len, query_request);
  buf_.clear();

  if (parse_ret < 0) {
    response->add_body(R"({"ret":"parse_error", "results":[]})""\n");
    response->send(400, NULL);
    LOG_INFO(logger, "[query:%s] error=parse_error, latency=%ldus", request_id.c_str(), watch.get_ticks_us());
    return;
  }

  // run query
  std::unique_ptr<QueryResult> result = executor_->execute(query_request);
  if (!result || result->is_error_status()) {
    response->add_body(R"({"ret":"query_error", "results":[]})""\n");
    response->send(500, NULL);
    LOG_INFO(logger, "[query:%s] REQ_STAT error=query_error, latency=%ldus", request_id.c_str(), watch.get_ticks_us());
    return;
  }

  std::ostringstream os; // output
  os  << R"({"ret":"success")"
      << R"(,"results":[)";
  bool first = true; // avoid trailing comma
  for (auto& r: result->get_results()) {
    if (first) {
      first = false;
    } else {
      os  << ',';
    }
    os  << R"({"uuid":")" << r.first << '"'
        << R"(,"score":)" << r.second << "}";
  }
  os << "]}\n";

  response->add_body(os.str());
  response->send(200, NULL);

  LOG_INFO(logger, "[query:%s] REQ_STAT ret=success, count=%zu, latency=%ldus",
      request_id.c_str(), result->get_results().size(), watch.get_ticks_us());
}

} /* namespace redgiant */
