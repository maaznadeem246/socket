/*    
bool send (
      Router* router,
      const String& seq,
      const String& data,
      const char* bytes,
      size_t size
    ) {
      #if defined(__APPLE__)
        if (seq.size() > 0 && seq != "-1" && [router->schemeTasks has: seq]) {
          auto task = [router->schemeTasks get: seq];
          auto msg = data;
          [router->schemeTasks remove: seq];

          #if !__has_feature(objc_arc)
            [task retain];
          #endif

          dispatch_async(dispatch_get_main_queue(), ^{
            auto headers = [[NSMutableDictionary alloc] init];
            auto length = bytes ? size : msg.size();

            headers[@"access-control-allow-origin"] = @"*";
            headers[@"access-control-allow-methods"] = @"*";
            headers[@"content-length"] = [@(length) stringValue];

            NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc]
               initWithURL: task.request.URL
                statusCode: 200
               HTTPVersion: @"HTTP/1.1"
              headerFields: headers
            ];

            [task didReceiveResponse: response];

            if (bytes) {
              auto data = [NSData dataWithBytes: bytes length: size];
              [task didReceiveData: data];
            } else if (msg.size() > 0) {
              auto string = [NSString stringWithUTF8String: msg.c_str()];
              auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
              [task didReceiveData: data];
            }

            [task didFinish];
            #if !__has_feature(objc_arc)
              [headers release];
              [response release];
            #endif
          });

          return true;
        }

      #endif

      return false;
    }
    */
