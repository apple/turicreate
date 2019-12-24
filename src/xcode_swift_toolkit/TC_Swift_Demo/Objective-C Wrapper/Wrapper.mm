//
//  Wrapper.m
//  TC_Swift_Demo
//
//  Created by Hoyt Koepke on 12/23/19.
//  Copyright © 2019 Hoyt Koepke. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "TC_Swift_Demo/TC_Swift_Demo-Swift.h"

#include <model_server_v2/model_base.hpp>
#include <model_server_v2/model_server.hpp>
#include <model_server_v2/registration.hpp>


namespace turi {
 
    // TODO: Make an Objective-C variant converter library.
    template <>
    struct variant_converter<NSString*, void> {
      static constexpr bool value = true;
        NSString* _Nonnull get(const variant_type& val) {
            return @(variant_get_ref<flexible_type>(val).get<flex_string>().c_str());
        }
        variant_type set(NSString* val) {
            return to_variant([val UTF8String]);
        }
    };

class Wrapper : public turi::v2::model_base {
public:
    
    SwiftMLClass* m;
    
    Wrapper() {
        m = [[SwiftMLClass alloc] init];
        
        register_method("append_string", &Wrapper::append_string, "s");
        register_method("get_string", &Wrapper::get_string);
    }
    
    const char* name() const override {
        return "SwiftWrapperTest";
    }

    void append_string(NSString* _Nonnull s) {
        [m append_stringWithX:s];
    }
    
    NSString* get_string() const {
        NSString* _Nonnull r = [m get_string];
        return r;
    }

};

REGISTER_MODEL(Wrapper);

}
