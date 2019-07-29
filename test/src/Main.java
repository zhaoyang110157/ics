
import javax.script.ScriptEngine;
import javax.script.ScriptEngineFactory;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;
import java.io.*;
import java.io.IOException;
import java.sql.SQLException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import javax.script.Invocable;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
public class Main {
/*
    public static void main(String[] args) {
        try{
            ScriptEngineManager m = new ScriptEngineManager();
//获取JavaScript执行引擎
            ScriptEngine engine = m.getEngineByName("JavaScript");
            //使用管道流，将输出流转为输入流
            PipedReader prd = new PipedReader();
            PipedWriter pwt = new PipedWriter(prd);
//设置执行结果内容的输出流
            engine.getContext().setWriter(pwt);
//js文件的路径
            String strFile = Thread.currentThread().getClass().getResource("/").getPath() + "/Arrange.js";
            Reader reader = new FileReader(new File(strFile));
            engine.eval(reader);
            BufferedReader br = new BufferedReader(prd);
//开始读执行结果数据
            String str = null;
            while ((str = br.readLine()) != null && str.length() > 0) {
                System.out.println(str);
            }
            br.close();
            pwt.close();
            prd.close();
        }catch (Exception e) {
            // 处理 Class.forName 错误
            e.printStackTrace();
        } finally {
            // 关闭资源
        }
    }*/
private static String readJSFile() throws Exception {
    StringBuffer script = new StringBuffer();
    File file = new File("D:\\lessons\\summer19\\project\\test\\src\\Arrange.js");
    FileReader filereader = new FileReader(file);
    BufferedReader bufferreader = new BufferedReader(filereader);
    String tempString = null;
    while ((tempString = bufferreader.readLine()) != null) {
        script.append(tempString).append("\n");
    }
    bufferreader.close();
    filereader.close();
    return script.toString();
}
private static void testJSFile() throws Exception {
    List<CourseSim> courseSims = new ArrayList<>();
    List<ClassInfoSim> classInfoSims = new ArrayList<>();
    List<ClassSegment> classSegments = new ArrayList<>();
    List<CourseSim> courseSims1 = new ArrayList<>();
    List<ClassInfoSim> classInfoSims1 = new ArrayList<>();
    List<ClassSegment> classSegments1 = new ArrayList<>();


    ClassSegment classSegment = new ClassSegment();
    ClassInfoSim classInfoSim = new ClassInfoSim();
    CourseSim courseSim = new CourseSim();
    ClassSegment classSegment1 = new ClassSegment();
    ClassInfoSim classInfoSim1 = new ClassInfoSim();
    CourseSim courseSim1 = new CourseSim();

    classSegment.setBegin_sec(1);
    classSegment.setEnd_sec(2);
    classSegment.setBegin_week(1);
    classSegment.setEnd_week(16);
    classSegment.setOdd_or_even('b');
    classSegment.setWeek(1);
    classSegments.add(classSegment);

    classInfoSim.setClassname("1-1");
    classInfoSim.setSegments(classSegments);
    classInfoSims.add(classInfoSim);

    classSegment1.setBegin_sec(1);
    classSegment1.setEnd_sec(2);
    classSegment1.setBegin_week(1);
    classSegment1.setEnd_week(16);
    classSegment1.setOdd_or_even('b');
    classSegment1.setWeek(3);
    classSegments1.add(classSegment1);

    classInfoSim1.setClassname("1-2");
    classInfoSim1.setSegments(classSegments1);

    classInfoSims.add(classInfoSim1);

    courseSim.setCourse_id("1");
    courseSim.setRequired(true);
    courseSim.setClasses(classInfoSims);

    courseSims.add(courseSim);

    System.out.println(courseSims.get(0).getClasses().size());

    ScriptEngineManager mgr = new ScriptEngineManager();
    ScriptEngine engine = mgr.getEngineByName("javascript");
    engine.eval(readJSFile());
    Invocable inv = (Invocable) engine;
    Object res =  inv.invokeFunction("arrange",courseSims);
}

    public static void main(String[] args) throws Exception {
        testJSFile();
    }
};