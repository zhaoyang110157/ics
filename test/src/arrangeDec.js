
var mandatory=[],optional=[],mResult=[],mClass = [],Result = [];
function conflict(data) {
    //这里拿到的是class，需要判定是否冲突，需要打开
    if(mClass.length === 0)
        return false;
    var segments = data.segments;
    for(var i=0;i<mClass.length;++i){
        var tmp = mClass[i].segments;
        for(var j =0; j<tmp.length ; ++j){
            for(var h = 0 ; h<segments.length;++h){
                if(segments[h].week !== tmp[j].week ){
                    continue;
                }
                else{
                    if(segments[h].begin_sec < tmp[j].end_sec || segments[h].end_sec > tmp[j].begin_sec)
                        continue;
                    else{
                        if(segments[h].begin_week < tmp[h].end_week || segments[h].end_week >tmp[j].begin_week)
                            continue;
                        else{
                            if(segments[h].odd_or_even === 'b' || tmp[j].odd_or_even === 'b' || segments[h].odd_or_even === tmp[j].odd_or_even)
                                return false;
                            else continue;
                        }
                    }
                }
            }
        }
    }
    return true;
}
function loopOptional(num) {
    if(num === 0){
        Result.push(mClass);
        return;
    }
    var data = optional.pop();
    var classes = data.classes;
    for(var i = 0;i<classes.length;++i){
        if(conflict(classes[i]) !== true){
            mClass.push(classes[i]);
            loopOptional(num-1);
            mClass.pop();
        }
        else{
            loopOptional(num-1);
        }
    }
    optional.push(data);
}
function loopMandatory(num) {
    if(num === 0 ){
        var tmp = [];
        for(var i = 0;i<mClass.length;++i)
            tmp.push(mClass[i]);
        mResult.push(tmp);

        return;
    }
    var data = mandatory.pop();
    var classes = data.classes;
    for(var i = 0; i< classes.length ; ++i){
        if(!conflict(classes[i]) ){
            mClass.push(classes[i]);
            loopMandatory(num-1);
            mClass.pop();
        }
    }
    mandatory.push(data);
}
function arrangeMandatory(){
    var num = mandatory.length;
    loopMandatory(num);
}
function arrangeOptional() {
    if(mResult.length === 0 ){
        mClass.splice();
        mResult.splice();
        mandatory = optional;
        arrangeMandatory();
        Result = mResult;
    }
    else{
        var num = optional.length;
        if(num === 0){
            print(mResult.length)
            print(mResult[0])
            print(mResult[1])
            return mResult;
        }

        else
            for(var i = 0 ; i < mResult.length ;++i){
                mClass = mResult[i];
                loopOptional(num);
            }
    }
    return Result;
}
function arrange(data) {
    print(data);
    print(data[0].classes)
    print(data[0].classes[1].segments)
    print(data[0].classes[0].segments)
    for(var i = 0; i< data.length;++i){
        if(data[i].required){
            mandatory.push(data[i]);
        }
        else
            optional.push(data[i]);
    }
    if(mandatory.length === 0){
        return arrangeOptional();
    }
    else{
        arrangeMandatory();
        if(mResult === null)
            return null;
        else{
            return arrangeOptional();
        }
    }

}