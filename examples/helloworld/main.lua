-- most simple service

function luarest_init(app)
  app:register(luarest.HTTP_METHOD_GET, "/hello", on_hello)
end

function on_hello(headers, params, body)
  local msg = "hello luarest"
  return luarest.HTTP_RESPONSE_OK, luarest.CONTENT_TYPE_PLAIN, msg
end
