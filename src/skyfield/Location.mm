// Location.mm
#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#import "Location.h"

@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
@property (nonatomic, strong) CLLocationManager *manager;
@property (nonatomic, assign) BOOL done;
@property (nonatomic, assign) GeoLocation result;
@end

@implementation LocationDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _done = NO;
        _result.valid = false;
    }
    return self;
}

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray<CLLocation *> *)locations
{
    CLLocation *loc = [locations lastObject];
    if (!loc) return;

    _result.latitudeDeg  = loc.coordinate.latitude;
    _result.longitudeDeg = loc.coordinate.longitude;
    _result.altitudeM    = loc.altitude;
    _result.valid        = true;

    _done = YES;
    [manager stopUpdatingLocation];
}

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
    _done = YES;
}

@end


GeoLocation getCurrentLocation()
{
    LocationDelegate *delegate = [[LocationDelegate alloc] init];
    delegate.manager = [[CLLocationManager alloc] init];
    delegate.manager.delegate = delegate;

    // Request permission
    [delegate.manager requestWhenInUseAuthorization];

    // Start updates
    [delegate.manager startUpdatingLocation];

    // Run a temporary runloop for up to 3 seconds
    NSDate *timeout = [NSDate dateWithTimeIntervalSinceNow:3.0];
    while (!delegate.done && [timeout timeIntervalSinceNow] > 0) {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    return delegate.result;
}

/*@interface LocationDelegate : NSObject <CLLocationManagerDelegate>
@property (nonatomic, strong) CLLocationManager *manager;
@property (nonatomic, assign) BOOL done;
@property (nonatomic, assign) double lat;
@property (nonatomic, assign) double lon;
@property (nonatomic, assign) double alt;
@end

@implementation LocationDelegate

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _done = NO;
        _lat = 0.0;
        _lon = 0.0;
        _alt = 0.0;
    }
    return self;
}

- (void)locationManager:(CLLocationManager *)manager
     didUpdateLocations:(NSArray<CLLocation *> *)locations
{
    CLLocation *loc = [locations lastObject];
    if (loc)
    {
        self.lat = loc.coordinate.latitude;
        self.lon = loc.coordinate.longitude;
        self.alt = loc.altitude;
        self.done = YES;
        [self.manager stopUpdatingLocation];
    }
}

- (void)locationManager:(CLLocationManager *)manager
       didFailWithError:(NSError *)error
{
    NSLog(@"Location error: %@", error);
    self.done = YES;
    [self.manager stopUpdatingLocation];
}

@end

GeoLocation getCurrentLocation()
{
    @autoreleasepool
    {
        GeoLocation result;
        result.latitudeDeg  = 0.0;
        result.longitudeDeg = 0.0;
        result.altitudeM    = 0.0;
        result.valid        = false;

        LocationDelegate *delegate = [[LocationDelegate alloc] init];
        CLLocationManager *manager = [[CLLocationManager alloc] init];
        delegate.manager = manager;
        manager.delegate = delegate;
        manager.desiredAccuracy = kCLLocationAccuracyBest;

        if ([CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined)
        {
            [manager requestWhenInUseAuthorization];
        }

        [manager startUpdatingLocation];

        // Simple polling loop with timeout (~10s)
        const int maxLoops = 100;
        for (int i = 0; i < maxLoops; ++i)
        {
            if (delegate.done)
                break;
            [NSThread sleepForTimeInterval:0.1];
        }

        if (delegate.done)
        {
            result.latitudeDeg  = delegate.lat;
            result.longitudeDeg = delegate.lon;
            result.altitudeM    = delegate.alt;
            result.valid        = true;
        }

        return result;
    }
}*/

