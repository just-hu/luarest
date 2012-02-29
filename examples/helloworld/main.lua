-- most simple service

function luarest_init(service)
  service:register(luarest.HTTP_METHOD_GET, "/hello", on_hello)
end

function on_hello(headers, params, body)
  return(luarest.HTTP_RESPONSE_OK, "hello luarest")
end