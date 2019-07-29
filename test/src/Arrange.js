/*
    var result = [] , courses = []  , n = 0 , mandatory = [] , mresult = [] , mtmp = [] ,    msegs = [],  optional = [] ,oresult = [];
    //  最终结果      要拍的所有课程  课程总数   必选            必选排课结果    必须排课中间变量             可选课程

    //判定能否加入
    function add(classes,mseg1){
        var mseg = mseg1;//防止变量修改
        var segments = classes.segments;
        var ilength = segments.length;
        print("segments's length "+ilength);
        for(var i = 0; i < ilength ; ++i){
            var mlength = mseg.length;
            var week = segments[i].week;
            var begin_sec = segments[i].begin_sec;
            var end_sec = segments[i].end_sec;
            var begin_week = segments[i].begin_week;
            var end_week = segments[i].end_week;
            var odd_or_even = segments[i].odd_or_even;
            print("segment "+ i + " "+ week + " " + begin_sec + " " +end_sec +" "+begin_week+" "+end_week+ " "+ odd_or_even);
            if(mlength === 0 ){
                mseg.push(segments[i]);
                continue;
            }
            for(var j = 0; j < mlength ; ++j) {
                print("check in")
                if (mseg[j].week < week) {
                    if (j == mlength - 1) {
                        mseg.push(segments[i]);
                        print("reach the  tail")
                        break;//到尾巴了，还没有找到冲突，加了
                    }
                    continue;//这个seg肯定影响不到插入，下一个
                }

                if (mseg[j].week > week) {
                    mseg.splice(j, 0, segments[i])
                    print("find the place")
                    break;//找到中间了，没有冲突，加了
                }

                //同一工作日
                if (begin_sec > mseg[j].end_sec || end_sec < mseg[j].begin_sec || begin_week > mseg[i].end_week || end_week < mseg[i].begin_week) {
                    //搭不上边      课程数没有重合 或者 周数没有重合
                    continue;
                }

                if(odd_or_even == 'b' || mseg[i].odd_or_even == 'b' || odd_or_even == mseg[i].odd_or_even)
                    return null;
                    //bomb
                else continue;
            }
        }
        return mseg;
    }

    //dfs 查找必修排课的一切可能性
    function loopMandatory(num,mseg){
        //madantory 里面是课 课下面是classes class 下面是segments
        for(var i = 0 ; i<mresult.length;++i){
            print(mresult[i].classes);
        }
        if(this.mandatory.length == 0)//没选必选
            return ;

        if(num === 0){
            this.mresult.push(this.mtmp); //收尾
            this.msegs.push(mseg);//记录已选时间段，方便可选排课
            return;
        }

        var tmp = this.mandatory[0];//tmp是课程
        this.mandatory.splice(0,1);
        this.mandatory.push(tmp);//塞回去
        var classes = this.mandatory[0].classes;//拿到classes
        //取第一门必修，并从必选队列剔除第一门

        //处理这一门的众多class
        var length = classes.length;
       // print("remain "+ num + " to handle "+length);
        for(var i = 0 ; i < length ; ++i){
            var t = [];
            t.push(classes[i]);
            tmp.classes = t;
           // print("segments' length " + tmp.classes[0].segments.length)
            this.mtmp.push(tmp);//课程放进mtmp
            var tmseg = add(classes[i], mseg );
            if(tmseg != null)
                this.loopMandatory(num-1,tmseg);
            this.mtmp.pop();
        }

    }

    function loopOptional(num,mseg){
        if(num === 0){
            this.result.push(this.mtmp);
            return;
        }

        var tmp = optional[0];
        this.optional.splice(0.1);
        this.optional.push(tmp);//塞回去
        var classes = this.optional[0].classes;
        //取第一门必修，并从必选队列剔除第一门

        //处理这一门的众多class
        var length = classes.length;

        for(var i = 0 ; i < length ; ++i){
            var t = [];
            t.push(classes[i]);
            tmp.classes = t;
            this.mtmp.push(tmp);
            var tmseg = add(classes[i], mseg );
            if(tmseg != null)
                this.loopMandatory(num-1,tmseg);
            this.mtmp.pop();
        }
    }

    //安排一下比选课
    function arrangeMandatory(){
        var num = this.mandatory.length;
        var seg = [];
       // print("start to arrange Mandatory "+num);
        loopMandatory(num,seg);
    }

    //安排一下可选课程
   function arrangeOptional( mres,mseg){
        this.mtmp = mres;
        var num = this.optional.length;
        loopOptional(num,mseg);
    }


    //主函数
   function arrange(data){
        n = data.length;
        for(var i = 0 ; i < n ; ++i)
        {
            var tmp = data[i];
         //   print("differ the "+i+ " element " + tmp.course_id + " " + tmp.required);
            if(tmp.required ===true){
                this.mandatory.push(tmp);
           //     print("add "+i + " to the mandatory "+this.mandatory.length + " " +tmp.classes.length);
            }
            else {
                this.optional.push(tmp);
            //    print("add "+i + " to the optional "+this.optional.length);
            }
        }//分开必选和可选。
        arrangeMandatory();
        //必选安排失败

        if(this.mandatory.length != 0){
            if(this.mresult.length == 0)
                return null;

            for(var i = 0 ; i < this.mresult.length ; ++i){
                arrangeOptional( this.mresult[i] ,this.msegs[i]);
            }
        }
        else arrangeOptional([],[]);
        var tmp =  this.mresult.length;
        return tmp;
    }*/


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