import java.util.List;

public class CourseSim {
    private String course_id;
    private Boolean required;
    private List<ClassInfoSim> classes;



    public Boolean getRequired() {        return required;    }
    public void setRequired(Boolean year) {        this.required = year;    }

    public void setCourse_id(String course_id) {        this.course_id = course_id;    }
    public String getCourse_id() {
        return course_id;
    }

    public List<ClassInfoSim> getClasses() {
        return classes;
    }
    public void setClasses(List<ClassInfoSim> classSegments) {
        this.classes = classSegments;
    }
}
