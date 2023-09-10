#define GL_SILENCE_DEPRECATION
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <fstream>
#include <GLUT/GLUT.h>
using namespace std;

#define RGB(a,b,c) (a/255.0),(b/255.0),(c/255.0)
#define DARK RGB(0,0,0) // 0
#define LT_BLUE RGB(251,185,207) // 1
#define DK_BLUE RGB(255,0,0) // 2
#define DK_YELLOW RGB(255,255,255) // 3
#define LT_YELLOW RGB(0,255,255) // 4
#define GREEN RGB(164,225,202) // 5
#define PURPLE RGB(128,0,128) // 6
#define RED RGB(255,0,0) //7
#define INFO_FONT GLUT_BITMAP_TIMES_ROMAN_24
const int PI = 3.1415926536;

struct HalfEdge;
struct Vertex;
struct Face;
struct quad;

struct Vertex{
    float x,y,z;
    Vertex(){
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
    Vertex(float a,float b,float c):x(a),y(b),z(c){}
    Vertex operator+(const Vertex &tar){
        return Vertex(x + tar.x , y + tar.y, z + tar.z);
    }
    Vertex operator/(float a){
        return Vertex(x / a , y / a , z / a);
    }
    Vertex operator*(float a){
        return Vertex(x * a , y * a , z * a);
    }
    Vertex& operator=(const Vertex &tar){
        if(this == &tar){
            return *this;
        }
        x = tar.x;
        y = tar.y;
        z = tar.z;
        return *this;
    }
};
 
struct HalfEdge{
    int origin;
    struct HalfEdge* next;
    struct HalfEdge* opposite;
    int IncFace;
};

struct Face{
    int order[3];
    Face(){
        order[0] = -1;
        order[1] = -1;
        order[2] = -1;
    }
    Face(int a,int b,int c){
        order[0] = a;
        order[1] = b;
        order[2] = c;
        halfedge = nullptr;
    }
    HalfEdge* halfedge;
};

struct quad{
    int order[4];
    quad(){
        order[0] = -1;
        order[1] = -1;
        order[2] = -1;
        order[3] = -1;
    }
    quad(int a,int b,int c,int d){
        order[0] = a;
        order[1] = b;
        order[2] = c;
        order[3] = d;
    }
    HalfEdge* halfedge;
};

class ObjLoader
{
public:
    ObjLoader(string filename,bool with_slash);//读取函数
    HalfEdge* findOriginEdge(int v);//找到从该定点出发的一条半边
    void initEdge();//初始化半边
    void Draw(int mode);//绘制函数
    void loop_subdivision();//细分一次
    void doo_sabin(int cur_time);
    void pre_catmull();
    void catmull();
    void destroy_edge();
    void reset();
    pair<int,int> get_st_ed(HalfEdge* tar);
    pair<int,int> get_pair_ab(int a,int b){
        return pair<int,int>(a,b);
    }
    int get_vertex_number(){
        return n_node;
    }
private:
    // 存放半边
    vector<HalfEdge*> edge;
    // 存放顶点集合
    vector<Vertex> v;
    // 存放面集合
    vector<Face> f;
    vector<quad> q;

    vector<Face> origin_face;
    vector<Vertex> origin_vec;

    int e_num;
    int n_node,n_face,n_quad,n_edge;
};

HalfEdge* ObjLoader::findOriginEdge(int v){
    for(int k=0;k<n_edge;k++)
    {
        if(edge[k]->origin==v)
            return edge[k];
    }
    return nullptr;
}
pair<int,int> ObjLoader::get_st_ed(HalfEdge* tmp){
    HalfEdge *oppo = tmp->opposite;
    pair<int,int> out(tmp->origin,oppo->origin);
    return out;
}

void ObjLoader::destroy_edge(){
    for(auto &p_edge:edge){
        delete p_edge;
    }
    edge.clear();
    n_edge = 0;
}

void ObjLoader::reset(){
    for(auto p_edge:edge){
        delete p_edge;
    }
    edge.clear();
    v.assign(origin_vec.begin(),origin_vec.end());
    f.assign(origin_face.begin(),origin_face.end());
    q.clear();
    n_quad = 0;
    n_node = v.size();
    n_face = f.size();
    this->initEdge();
}

void ObjLoader::initEdge(){
    if(!edge.empty()){
        this->destroy_edge();
    }
    map< pair<int,int>, HalfEdge* > edge_keymap;
    for(int i = 0;i < q.size();i++){
        HalfEdge* quad_halfedge[4];

        for(int j =0;j < 4;j++){
            int vid1 = q[i].order[j%4];
            int vid2 = q[i].order[(j+1)%4];
            cout << vid1 <<"   "<<vid2<<endl;
            pair<int,int> tmp_pair1(vid1,vid2);
            pair<int,int> tmp_pair2(vid2,vid1);
            if(edge_keymap.find(tmp_pair1) == edge_keymap.end()){
                HalfEdge* cur_hedge = new HalfEdge();
                HalfEdge* cur_oppo = new HalfEdge();

                cur_hedge->origin = vid1;
                cur_oppo->origin = vid2;
                cur_hedge->opposite = cur_oppo;
                cur_oppo->opposite = cur_hedge;

                edge_keymap[tmp_pair1] = cur_hedge;
                edge_keymap[tmp_pair2] = cur_oppo;

                quad_halfedge[j]= cur_hedge;
                edge.push_back(cur_hedge);
                edge.push_back(cur_oppo);
            }
            else{
                quad_halfedge[j] = edge_keymap[tmp_pair1];
            }
        }
        for(int j = 0;j < 4;j++){
            quad_halfedge[j]->IncFace = i;
        }
        quad_halfedge[0]->next = quad_halfedge[1];
        quad_halfedge[1]->next = quad_halfedge[2];
        quad_halfedge[2]->next = quad_halfedge[3];
        quad_halfedge[3]->next = quad_halfedge[0];

        q[i].halfedge = quad_halfedge[0];
    }
    for(int i = 0;i < f.size();i++){
        HalfEdge* tri_halfedge[3];

        for(int j = 0;j < 3;j++){
            int vid1 = f[i].order[j%3],vid2 = f[i].order[(j+1)%3];
            pair<int,int> tmp_pair1(vid1,vid2);
            if(edge_keymap.find(tmp_pair1) == edge_keymap.end()){
                HalfEdge* cur_edge = new HalfEdge();
                HalfEdge* cur_oppo = new HalfEdge();

                cur_edge->origin = vid1;
                cur_oppo->origin = vid2;
                cur_edge->opposite = cur_oppo;
                cur_oppo->opposite = cur_edge;

                pair<int,int> tmp_pair2(vid2,vid1);
                edge_keymap[tmp_pair1] = cur_edge;
                edge_keymap[tmp_pair2] = cur_oppo;

                tri_halfedge[j]= cur_edge;
                edge.push_back(cur_edge);
                edge.push_back(cur_oppo);
            }
            else{
                tri_halfedge[j] = edge_keymap[tmp_pair1];
            }
        }
        for(int j = 0;j < 3;j++){
            tri_halfedge[j]->IncFace = n_quad + i;
        }
        tri_halfedge[0]->next = tri_halfedge[1];
        tri_halfedge[1]->next = tri_halfedge[2];
        tri_halfedge[2]->next = tri_halfedge[0];

        f[i].halfedge = tri_halfedge[0];
    }
    n_edge = edge.size();
}

void ObjLoader::loop_subdivision(){
    this->initEdge();
    vector<Vertex> vertex2;
    vector<Face> face2;
    HalfEdge* he;
    int n;
    float p_sumx,p_sumy,p_sumz;
    float px,py,pz;
    float beta;
    cout<<"细分开始"<<endl;
    for(int i=0;i<n_node;i++)//旧点更新
    {
        he = findOriginEdge(i);
 
        if(he!=NULL)
        {
            n=0;
            p_sumx=0;
            p_sumy=0;
            p_sumz=0;
            HalfEdge* e=new HalfEdge();
            e=he->next;
            int p0=e->origin;
 
            while(e->next->origin!=p0)
            {
                n++;
                p_sumx+=v[e->next->origin].x;
                p_sumy+=v[e->next->origin].y;
                p_sumz+=v[e->next->origin].z;
                HalfEdge* te=new HalfEdge();
                te=e->next->opposite;
                e=te->next;
            }
            n++;
            p_sumx+=v[p0].x;
            p_sumy+=v[p0].y;
            p_sumz+=v[p0].z;
            beta=1/(double)n*(0.625-pow(0.375+0.25*cos(2*PI/n),2));
 
            px=(1-n*beta)*v[i].x+beta*p_sumx;
            py=(1-n*beta)*v[i].y+beta*p_sumy;
            pz=(1-n*beta)*v[i].z+beta*p_sumz;
 
            Vertex vec;
            vec.x=px;
            vec.y=py;
            vec.z=pz;
            vertex2.push_back(vec);
        }
    }
    // int map1[n_node][n_node];
    vector< vector<int> > map1(n_node,vector<int>(n_node));
    for(int i = 0; i < n_node;i++){
        for(int j = 0; j < n_node;j++){
            map1[i][j] = 0;
        }
    }
    float qx,qy,qz;
 
    for(int i=0;i<n_edge;i++)//新点生成
    {
        if(!map1[edge[i]->origin][edge[i]->next->origin])
        {
            int p=edge[i]->origin;
            int pi=edge[i]->next->origin;
            int pi1=edge[i]->next->next->origin;
            int pi0=edge[i]->opposite->next->next->origin;
            qx=0.375*(v[p].x+v[pi].x)+0.125*(v[pi1].x+v[pi0].x);
            qy=0.375*(v[p].y+v[pi].y)+0.125*(v[pi1].y+v[pi0].y);
            qz=0.375*(v[p].z+v[pi].z)+0.125*(v[pi1].z+v[pi0].z);
 
            Vertex vec;
            vec.x=qx;
            vec.y=qy;
            vec.z=qz;
            vertex2.push_back(vec);
 
            map1[edge[i]->origin][edge[i]->next->origin]=vertex2.size()-1;
            map1[edge[i]->next->origin][edge[i]->origin]=vertex2.size()-1;
        }
    }

    for(int i=0;i<n_face;i++)//新面
    {
        int a,b,c,d,e,tmp;
        a=f[i].order[0];
        b=f[i].order[1];
        c=f[i].order[2];
        d=map1[a][b];
        e=map1[b][c];
        tmp=map1[a][c];
 
        Face f2;
 
        f2.order[0]=a;
        f2.order[1]=d;
        f2.order[2]=tmp;
        face2.push_back(f2);
 
        f2.order[0]=d;
        f2.order[1]=b;
        f2.order[2]=e;
        face2.push_back(f2);
 
        f2.order[0]=d;
        f2.order[1]=e;
        f2.order[2]=tmp;
        face2.push_back(f2);
 
        f2.order[0]=tmp;
        f2.order[1]=e;
        f2.order[2]=c;
        face2.push_back(f2);
    }

    n_face=face2.size();
    n_node=vertex2.size();
    cout<<n_node<<" "<<n_face<<endl;
 
    // int map2[n_node][n_node];
    vector< vector<int> > map2(n_node,vector<int>(n_node));
    //int * map2=new int[n_node][n_node];
    for(int i=0;i<n_node;i++){
        for(int j=0;j<n_node;j++){
            map2[i][j]=-1;
        }
    }
    cout<<"map2="<<sizeof(map2[0])/sizeof(int)<<endl;

    v.assign(vertex2.begin(),vertex2.end());
    f.assign(face2.begin(),face2.end());
 
cout<<"完成一次细分"<<endl;
cout<<n_node<<" "<<" "<<n_face<<endl;
}

void ObjLoader::doo_sabin(int cur_time){
    if(cur_time == 1){
        this->pre_catmull();
    }
    this->initEdge();
    vector<Vertex> vertex2;
    vector<Face> face2;
    vector<quad> quad2;
    map< pair<int,int>, int> edge_map;

    for(int i = 0;i < n_quad;i++){
        Vertex center;
        for(int j = 0;j < 4;j++){
            center = center + v[q[i].order[j]];
        }
        center = center / 4.0f;

        Vertex edge_mid[4];
        HalfEdge *ori_edge = q[i].halfedge;
        HalfEdge *cur_edge = ori_edge;
        int count = 0;
        do{
            HalfEdge *oppo_edge = cur_edge->opposite;
            edge_mid[count++] = (v[cur_edge->origin] + v[oppo_edge->origin]) / 2.0f;
            cur_edge = cur_edge->next;
        }while(cur_edge != ori_edge);

        int new_vid[4];
        cur_edge = ori_edge;
        for(int j = 0;j < 4;j++){
            Vertex new_vec = (v[q[i].order[j]] + center + edge_mid[(j+3)%4] +edge_mid[j]) / 4.0f;
            edge_map[get_st_ed(cur_edge)] = vertex2.size();
            new_vid[j] = vertex2.size();
            vertex2.push_back(new_vec);
            cur_edge = cur_edge->next;
        }
        quad2.push_back(quad(new_vid[0],new_vid[1],new_vid[2],new_vid[3]));
    }

    for(int i = 0; i < n_face;i++){
        Vertex center;
        for(int j = 0;j < 3;j++){
            center = center + v[f[i].order[j]];
        }
        center = center / 3.0f;//计算面中心点

        Vertex edge_mid[3];
        HalfEdge* ori_edge = f[i].halfedge;
        HalfEdge* cur_edge = ori_edge;
        int count = 0;
        do{
            HalfEdge *oppo_edge = cur_edge->opposite;
            edge_mid[count++] = (v[cur_edge->origin] + v[oppo_edge->origin]) / 2.0f;
            cur_edge = cur_edge->next;
        }while(cur_edge != ori_edge);//计算边中点

        int new_vid[3];
        cur_edge = ori_edge;
        for(int j = 0;j < 3;j++){
            Vertex new_vec = (v[f[i].order[j]] + center + edge_mid[(j+2)%3] + edge_mid[j]) / 4.0f;
            edge_map[get_st_ed(cur_edge)] = vertex2.size();
            new_vid[j] = vertex2.size();
            vertex2.push_back(new_vec);
            cur_edge = cur_edge->next;
        }
        face2.push_back(Face(new_vid[0],new_vid[1],new_vid[2]));
    }
    for(int i = 0;i < n_node;i++){
        HalfEdge* ori_edge = findOriginEdge(i);
        HalfEdge* cur_edge = ori_edge;
        vector<int> near_vec;
        do{
            near_vec.push_back(edge_map[get_st_ed(cur_edge)]);
            cur_edge = cur_edge->opposite->next;
        }while(cur_edge != ori_edge);

        if(near_vec.size() == 3){
            face2.push_back(Face(near_vec[2],near_vec[1],near_vec[0]));
        }
        else if(near_vec.size() == 4){
            quad2.push_back(quad(near_vec[3],near_vec[2],near_vec[1],near_vec[0]));
        }
        else{
            int maxium = near_vec.size();
            for(int i = 0;i < maxium - 2;i++){
                face2.push_back(Face(near_vec[0],near_vec[i+1],near_vec[i+2]));
            }
        }
    }

    set<HalfEdge*> edge_set;
    for(int i = 0;i < edge.size();i++){
        HalfEdge* cur_edge = edge[i];
        if(edge_set.find(cur_edge) == edge_set.end()){
            edge_set.insert(cur_edge);
            edge_set.insert(cur_edge->opposite);

            int v0 = edge_map[get_st_ed(cur_edge)];
            int v1 = edge_map[get_st_ed(cur_edge->next)];
            cur_edge = cur_edge ->opposite;
            int v2 = edge_map[get_st_ed(cur_edge)];
            int v3 = edge_map[get_st_ed(cur_edge->next)];
            quad2.push_back(quad(v3,v2,v1,v0));
        }
    }
    v.assign(vertex2.begin(),vertex2.end());
    q.assign(quad2.begin(),quad2.end());
    f.assign(face2.begin(),face2.end());
    n_node = v.size();
    n_quad = q.size();
    n_face = f.size();
}

void ObjLoader::pre_catmull(){
    map< pair<int,int>,int> node_map;
    for(int i = 0;i < n_face;i++){
        int vid0 = f[i].order[0];
        int vid1 = f[i].order[1];
        int vid2 = f[i].order[2];

        Vertex center = v[vid0] + v[vid1] + v[vid2];
        center = center / 3.0f;

        int order_center = v.size();
        v.push_back(center);

        Vertex edge_mid[3];
        edge_mid[0] = (v[vid0] + v[vid1]) / 2.0f;
        edge_mid[1] = (v[vid1] + v[vid2]) / 2.0f;
        edge_mid[2] = (v[vid2] + v[vid0]) / 2.0f;

        int order[3];
        if(node_map.find(get_pair_ab(vid0,vid1)) == node_map.end()){
            node_map[get_pair_ab(vid0,vid1)] = v.size();
            node_map[get_pair_ab(vid1,vid0)] = v.size();
            order[0] = v.size();
            v.push_back(edge_mid[0]);
        }
        else{
            order[0] = node_map[get_pair_ab(vid0,vid1)];
        }
        
        if(node_map.find(get_pair_ab(vid1,vid2)) == node_map.end()){
            node_map[get_pair_ab(vid1,vid2)] = v.size();
            node_map[get_pair_ab(vid2,vid1)] = v.size();
            order[1] = v.size();
            v.push_back(edge_mid[1]);
        }
        else{
            order[1] = node_map[get_pair_ab(vid1,vid2)];
        }

        if(node_map.find(get_pair_ab(vid2,vid0)) == node_map.end()){
            node_map[get_pair_ab(vid0,vid2)] = v.size();
            node_map[get_pair_ab(vid2,vid0)] = v.size();
            order[2] = v.size();
            v.push_back(edge_mid[2]);
        }
        else{
            order[2] = node_map[get_pair_ab(vid0,vid2)];
        }

        q.push_back(quad(vid0,order[0],order_center,order[2]));
        q.push_back(quad(vid1,order[1],order_center,order[0]));
        q.push_back(quad(vid2,order[2],order_center,order[1]));
    }
    n_node = v.size();
    n_quad = q.size();
    f.clear();
    n_face = f.size();
}

void ObjLoader::catmull(){
    if(n_face != 0){
        this->pre_catmull();
    }
    this->initEdge();

    for(int i = 0;i < n_quad;i++){
        Vertex center;
        for(int j = 0;j < 4;j++){
            center = center + v[q[i].order[j]];
        }
        center = center / 4.0f;
        v.push_back(center);
    }

    map< pair<int,int>, int> edge_map;
    for(int i = 0;i < n_quad;i++){
        HalfEdge *cur_edge = q[i].halfedge;
        for(int j = 0;j < 4;j++){
            if(edge_map.find(get_st_ed(cur_edge)) == edge_map.end()){
                Vertex vec3f;
                vec3f = vec3f + v[cur_edge->origin] +v[cur_edge->opposite->origin];
                vec3f = vec3f + v[cur_edge->IncFace + n_node] + v[cur_edge->opposite->IncFace + n_node];
                edge_map[get_st_ed(cur_edge)] = v.size();
                edge_map[get_st_ed(cur_edge->opposite)] = v.size();
                v.push_back(vec3f/4.0f);
            }
            cur_edge = cur_edge->next;
        }
    }
    vector<Vertex> vertex2(n_node);
    for(int i = 0;i < n_node;i++){
        HalfEdge* ori_edge = findOriginEdge(i);
        HalfEdge* cur_edge = ori_edge;

        if(ori_edge == nullptr){
            continue;
        }
        Vertex Q;
        Vertex R;
        int n = 0;
        do{
            n++;
            Q = Q + v[cur_edge->IncFace + n_node];
            R = R + v[edge_map[get_st_ed(cur_edge)]];
            cur_edge = cur_edge->opposite->next;
        }while(cur_edge != ori_edge);
        Q = Q / n;
        R = R / n;
        vertex2[i] = Q / n + R * 2.0f / n + v[i] * (float)(n - 3) / n;
    }

    vector<quad> quad2(n_quad * 4);
    int count = 0;
    for(int i = 0;i < n_quad;i++){
        int vid0 = q[i].order[0];
        int vid1 = q[i].order[1];
        int vid2 = q[i].order[2];
        int vid3 = q[i].order[3];

        HalfEdge* cur_edge = q[i].halfedge;
        int vid4 = edge_map[get_st_ed(cur_edge)];
        cur_edge = cur_edge->next;
        int vid5 = edge_map[get_st_ed(cur_edge)];
        cur_edge = cur_edge->next;
        int vid6 = edge_map[get_st_ed(cur_edge)];
        cur_edge = cur_edge->next;
        int vid7 = edge_map[get_st_ed(cur_edge)];
        int vid8 = i + n_node;

        quad2[count++] = quad(vid0,vid4,vid8,vid7);
        quad2[count++] = quad(vid4,vid1,vid5,vid8);
        quad2[count++] = quad(vid8,vid5,vid2,vid6);
        quad2[count++] = quad(vid7,vid8,vid6,vid3);
    }
    q.assign(quad2.begin(),quad2.end());
    for(int i = 0;i < n_node;i++){
        v[i] = vertex2[i];
    }
    n_quad = q.size();
    n_node = v.size();
}

ObjLoader::ObjLoader(string filename,bool with_slash)
{
    ifstream file(filename);
    string line;
    while (getline(file, line))
    {
        if (line.substr(0, 1) == "v")
        {
            Vertex read_tmp_vec;
            istringstream s(line.substr(2));
            s >> read_tmp_vec.x; s >> read_tmp_vec.y; s >> read_tmp_vec.z;
            v.push_back(read_tmp_vec);
        }
        else if (line.substr(0, 1) == "f")
        {
            Face read_tmp_face;
            char c;GLint tmp;
            istringstream vtns(line.substr(2));
            if (with_slash == true){
                vtns >> read_tmp_face.order[0] >> c >> tmp; vtns >> read_tmp_face.order[1] >> c >> tmp; vtns >> read_tmp_face.order[2] >> c >> tmp;
            }
            else{
                vtns >> read_tmp_face.order[0];vtns >> read_tmp_face.order[1]; vtns >> read_tmp_face.order[2];
            }
            for(int i = 0;i <= 2;i++){
                read_tmp_face.order[i]--;
            }
            read_tmp_face.halfedge = nullptr;
            f.push_back(read_tmp_face);
        }
    }
    n_face = f.size();
    n_node = v.size();
    n_quad = 0;
    file.close();
    origin_face.assign(f.begin(),f.end());
    origin_vec.assign(v.begin(),v.end());
}

void ObjLoader::Draw(int mode)
{
    glBegin(GL_TRIANGLES);//开始绘制
    for (int i = 0; i < f.size(); i++) {
        GLfloat VN[3];//法线
        //三个顶点
        Vertex a, b, c, normal;

        if (false) {
            cout << "ERRER::THE SIZE OF f IS NOT 3!" << endl;
        }
        else {
            GLint firstVertexIndex = f[i].order[0];//取出顶点索引
            GLint secondVertexIndex = f[i].order[1];
            GLint thirdVertexIndex = f[i].order[2];
            
            a.x = v[firstVertexIndex].x;//第一个顶点
            a.y = v[firstVertexIndex].y;
            a.z = v[firstVertexIndex].z;

            b.x = v[secondVertexIndex].x; //第二个顶点
            b.y = v[secondVertexIndex].y;
            b.z = v[secondVertexIndex].z;

            c.x = v[thirdVertexIndex].x; //第三个顶点
            c.y = v[thirdVertexIndex].y;
            c.z = v[thirdVertexIndex].z;


            GLfloat vec1[3], vec2[3], vec3[3];//计算法向量
            //(x2-x1,y2-y1,z2-z1)
            vec1[0] = a.x - b.x;
            vec1[1] = a.y - b.y;
            vec1[2] = a.z - b.z;

            //(x3-x2,y3-y2,z3-z2)
            vec2[0] = a.x - c.x;
            vec2[1] = a.y - c.y;
            vec2[2] = a.z - c.z;

            //(x3-x1,y3-y1,z3-z1)
            vec3[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
            vec3[1] = vec2[0] * vec1[2] - vec2[2] * vec1[0];
            vec3[2] = vec2[1] * vec1[0] - vec2[0] * vec1[1];

            GLfloat D = sqrt(pow(vec3[0], 2) + pow(vec3[1], 2) + pow(vec3[2], 2));

            VN[0] = vec3[0] / D;
            VN[1] = vec3[1] / D;
            VN[2] = vec3[2] / D;
            GLdouble heart[]={(a.x+b.x+c.x)/3.0,(a.y+b.y+c.y)/3.0,(a.z+b.z+c.z)/3.0};
            if (mode == 0){
                GLdouble color = fabs(VN[0]*heart[0] + VN[1]*heart[1] + VN[2]*heart[2]);
                glColor3f(color, color, color);
                glNormal3f(VN[0], VN[1], VN[2]);
                glVertex3f(a.x, a.y, a.z);//绘制三角面
                glVertex3f(b.x, b.y, b.z);
                glVertex3f(c.x, c.y, c.z);
            }
            else if(mode == 1){
                glNormal3f(VN[0], VN[1], VN[2]);
                glColor3f(cos(a.x), cos(a.y), cos(a.z));
                glVertex3f(a.x, a.y, a.z);//绘制三角面
                glColor3f(cos(b.x), cos(b.y), cos(b.z));
                glVertex3f(b.x, b.y, b.z);
                glColor3f(cos(c.x), cos(c.y), cos(c.z));
                glVertex3f(c.x, c.y, c.z);
            }else if(mode == 2){
                glNormal3f(VN[0], VN[1], VN[2]);
                glColor3f(VN[0]*heart[0],VN[1]*heart[1],VN[2]*heart[2]);
                glColor3f(VN[0],VN[1],VN[2]);
                glVertex3f(a.x, a.y, a.z);//绘制三角面
                glVertex3f(b.x, b.y, b.z);
                glVertex3f(c.x, c.y, c.z);
            }
            else if(mode == 3){
                glNormal3f(VN[0], VN[1], VN[2]);
                GLdouble color = fabs(VN[0]*a.x + VN[1]*a.y + VN[2]*a.z);
                glColor3f(color,color,color);
                glVertex3f(a.x, a.y, a.z);//绘制三角面
                color = fabs(VN[0]*b.x + VN[1] * b.y + VN[2] * b.z);
                glColor3f(color,color,color);
                glVertex3f(b.x, b.y, b.z);
                color = fabs(VN[0]*c.x + VN[1] * c.y + VN[2] * c.z);
                glColor3f(color,color,color);
                glVertex3f(c.x, c.y, c.z);
            }
        }
    }
    glEnd();

    glBegin(GL_QUADS);
    for(int i = 0;i < q.size();i++){
        GLfloat VN[3];
        Vertex a, b, c, d,normal;
        GLint one_index = q[i].order[0];//取出顶点索引
        GLint two_index = q[i].order[1];
        GLint three_index = q[i].order[2];
        GLint four_index = q[i].order[3];
            
        a = v[one_index];
        b = v[two_index];
        c = v[three_index];
        d = v[four_index];

        GLfloat vec1[3], vec2[3], vec3[3];//计算法向量
            //(x2-x1,y2-y1,z2-z1)
        vec1[0] = a.x - b.x;
        vec1[1] = a.y - b.y;
        vec1[2] = a.z - b.z;

            //(x3-x2,y3-y2,z3-z2)
        vec2[0] = a.x - c.x;
        vec2[1] = a.y - c.y;
        vec2[2] = a.z - c.z;

            //(x3-x1,y3-y1,z3-z1)
        vec3[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
        vec3[1] = vec2[0] * vec1[2] - vec2[2] * vec1[0];
        vec3[2] = vec2[1] * vec1[0] - vec2[0] * vec1[1];

        GLfloat D = sqrt(pow(vec3[0], 2) + pow(vec3[1], 2) + pow(vec3[2], 2));

        VN[0] = vec3[0] / D;
        VN[1] = vec3[1] / D;
        VN[2] = vec3[2] / D;
        GLdouble heart[]={(a.x+b.x+c.x)/3.0,(a.y+b.y+c.y)/3.0,(a.z+b.z+c.z)/3.0};
        if (mode == 0){
            GLdouble color = fabs(VN[0]*heart[0] + VN[1]*heart[1] + VN[2]*heart[2]);
            glColor3f(color, color, color);
            glNormal3f(VN[0], VN[1], VN[2]);
            glVertex3f(a.x, a.y, a.z);//绘制三角面
            glVertex3f(b.x, b.y, b.z);
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(d.x, d.y, d.z);
        }
        else if(mode == 1){
            glNormal3f(VN[0], VN[1], VN[2]);
            glColor3f(cos(a.x), cos(a.y), cos(a.z));
            glVertex3f(a.x, a.y, a.z);//绘制三角面
            glColor3f(cos(b.x), cos(b.y), cos(b.z));
            glVertex3f(b.x, b.y, b.z);
            glColor3f(cos(c.x), cos(c.y), cos(c.z));
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(d.x, d.y, d.z);
        }else if(mode == 2){
            glNormal3f(VN[0], VN[1], VN[2]);
            // glColor3f(VN[0]*heart[0],VN[1]*heart[1],VN[2]*heart[2]);
            glColor3f(VN[0],VN[1],VN[2]);
            glVertex3f(a.x, a.y, a.z);//绘制三角面
            glVertex3f(b.x, b.y, b.z);
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(d.x, d.y, d.z);
        }
        else if(mode == 3){
            glNormal3f(VN[0], VN[1], VN[2]);
            GLdouble color = fabs(VN[0]*a.x + VN[1]*a.y + VN[2]*a.z);
            glColor3f(color,color,color);
            glVertex3f(a.x, a.y, a.z);//绘制三角面
            color = fabs(VN[0]*b.x + VN[1] * b.y + VN[2] * b.z);
            glColor3f(color,color,color);
            glVertex3f(b.x, b.y, b.z);
            color = fabs(VN[0]*c.x + VN[1] * c.y + VN[2] * c.z);
            glColor3f(color,color,color);
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(d.x, d.y, d.z);
        }
    }
    glEnd();
}

ObjLoader obj1 = ObjLoader("pyramid.obj",false);
ObjLoader obj2 = ObjLoader("cube.obj",false);
ObjLoader obj3 = ObjLoader("bunny.obj",true);

char sub_way;
int status;
int cur_obj;
int cur_time;
int times;
int rotation;

inline void print(string s,int x,int y){
    glRasterPos2d(x,y);
    for(int i = 0;i < s.length();i++){
        glColor3f(RED);
        glutBitmapCharacter(INFO_FONT,s[i]);
    }
}

void initial(void){
//    glEnable(GL_LIGHTING);
//    glEnable(GL_LIGHT0);
    
    glShadeModel(GL_SMOOTH); // 设置着色模式 为 平滑
    glClearColor(0, 0, 0, 0.0);
    glClearDepth(1.0f); // 清空深度缓存
    
    glEnable(GL_DEPTH_TEST); //开启更新深度缓冲区
    glDepthFunc(GL_LEQUAL); //设置深度函数
    glEnable(GL_COLOR_MATERIAL); // 材质，可不用
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // 系统透视关系修正
    
    glMatrixMode(GL_PROJECTION);
        //opengl是一个状态机，要先清空之前的变换矩阵数据，所以每次视角操作时都要先变为单位矩阵
    glLoadIdentity();
        //使用透视变换，也可以使用gluPerspective函数，参数有所不同
    glFrustum(-2.0, 2.0, -2.0, 2.0, 2.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//绘画前清除缓冲
    glLoadIdentity();// 恢复初始坐标系
    
   GLfloat light_position[] = { 5.0, 5.0, 1.0, 0.0 }; // 点光源位置
   GLfloat mat_ambient[] = { 0.2, 0.2, 0.2, 1.0 }; // 环境光反射系数  // 泛光模型只启用1 其余设置为0
   GLfloat mat_diffuse[] = { 0.5, 0.5, 0.5, 1.0 }; // 漫反射系数    //lamber 模型启用1，2 其余设置为0
   GLfloat mat_specular[] = { 0.7, 0.7, 0.7, 1.0 }; // 镜面反射系数  //phong模型启用1，2，3，4
   GLfloat mat_shininess[] = { 10.0 }; // 高光指数

   glLightfv(GL_LIGHT0, GL_AMBIENT, mat_ambient);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, mat_diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, mat_specular);
   glLightfv(GL_LIGHT0, GL_SHININESS, mat_shininess);

   glLightfv(GL_LIGHT0, GL_POSITION, light_position);// 设置光源位置
    if (status == 0){
        string out = "Press \'s\' to start demo";
        glPushMatrix();
        glTranslatef(-0.6f,0,-5);
        glScalef(0.01f,0.01f,0.01f);
        print(out,0,0);
        glPopMatrix();
    }
    else if(status == 1){
        string out = "Choose which to demonstrate";
        glPushMatrix();
        glTranslatef(-0.9f,-1.5f,-5);
        glScalef(0.01f,0.01f,0.01f);
        print(out,0,0);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-2.0f,0.0f,-5);
        glScalef(1.3f,1.3f,1.3f);
        glRotatef(rotation,1,0,1);
        obj1.Draw(2);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-0.2f,-0.0f,-5);
        glScalef(1.2f,1.2f,1.2f);
        glRotatef(rotation,1,0,1);
        obj2.Draw(2);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(2.3f,-0.0f,-5);
        glRotatef(rotation,1,0,1);
        obj3.Draw(2);
        glPopMatrix();

    }else if(status == 2){
        string out = "Choose the subdivision way, Loop(l), Catmull(c), Doo-sabin(d)";
        glPushMatrix();
        glTranslatef(-1.8f,0.0f,-5);
        glScalef(0.01f,0.01f,0.01f);
        print(out,0,0);
        glPopMatrix();
    }
    else if(status == 3){
        string out = "Press s to proceed subdivision, press p to back to menu";
        glPushMatrix();
        glTranslatef(-1.8f,1.5f,-5);
        glScalef(0.01f,0.01f,0.01f);
        print(out,0,0);
        glPopMatrix();
        string sub_out;
        if(sub_way == 'l'){
            sub_out = "Current loop";
        }
        else if(sub_way == 'd'){
            sub_out = "Current doo-sabin";
        }
        else if(sub_way == 'c'){
            sub_out = "Current catmull-clark";
        }
        sub_out += " time is      "+ to_string(cur_time)+ "          Press r to reset";
        glPushMatrix();
        glTranslatef(-1.5f,-1.5f,-5);
        glScalef(0.01f,0.01f,0.01f);
        print(sub_out,0,0);
        glPopMatrix();
        if(cur_obj == 1){
            if(times == 0 && cur_time !=0){
                obj1.reset();
                cur_time = 0;
            }else if(cur_time < times){
                cur_time++;
                if(sub_way == 'l'){
                    obj1.loop_subdivision();
                }
                else if(sub_way == 'd'){
                    obj1.doo_sabin(cur_time);
                }
                else if(sub_way == 'c'){
                    obj1.catmull();
                }
            }else if(cur_time == times){
                glPushMatrix();
                glTranslatef(-0.0f,0.0f,-5);
                glScalef(2.0f,2.0f,2.0f);
                glScalef(1+cur_time/10.0f,1+cur_time/10.0f,1+cur_time/10.0f);
                glRotatef(rotation,1,0,1);
                obj1.Draw(2);
                glPopMatrix();
            }
        }else if(cur_obj == 2){
            if(times == 0 && cur_time != 0){
                obj2.reset();
                cur_time = 0;
            }else if(cur_time < times){
                cur_time++;
                if(sub_way == 'l'){
                    obj2.loop_subdivision();
                }
                else if(sub_way == 'd'){
                    obj2.doo_sabin(cur_time);
                }
                else if(sub_way == 'c'){
                    obj2.catmull();
                }
            }else if(cur_time == times){
                glPushMatrix();
                glTranslatef(-0.0f,-0.0f,-5);
                glScalef(1.3f,1.3f,1.3f);
                glScalef(1+cur_time/10.0f,1+cur_time/10.0f,1+cur_time/10.0f);
                glRotatef(rotation,1,0,1);
                obj2.Draw(2);
                glPopMatrix();
            }
        }else if(cur_obj == 3){
            if(times == 0 && cur_time != 0){
                obj3.reset();
                cur_time = 0;
            }else if(cur_time < times){
                cur_time++;
                if(sub_way == 'l'){
                    obj3.loop_subdivision();
                }
                else if(sub_way == 'd'){
                    obj3.doo_sabin(cur_time);
                }
                else if(sub_way == 'c'){
                    obj3.catmull();
                }
            }else if(cur_time == times){
                glPushMatrix();
                glTranslatef(0.5f,-0.0f,-5);
                glScalef(1+cur_time/10.0f,1+cur_time/10.0f,1+cur_time/10.0f);
                glRotatef(rotation,1,0,1);
                obj3.Draw(2);
                glPopMatrix();
            }
        }
    }
    rotation = (rotation%360) + 1;
    glutSwapBuffers();
}

void reshape(int width, int height){
    if(height == 0) height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 0.1f,100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyinput(unsigned char c,int x,int y){
    if (status == 0){
        if(c == 's'){
            status = 1;
        }
    }else if(status == 1){
        if(c == '1'){
            status = 2;
            cur_obj = 1;
        }else if(c == '2'){
            status = 2;
            cur_obj = 2;
        }else if(c == '3'){
            status = 2;
            cur_obj = 3;
        }
    }else if(status == 2){
        sub_way = c;
        status = 3;
    }else if(status == 3){
        if(c == 's'){
            times++;
        }else if(c == 'r'){
            times = 0;
        }else if(c == 'p'){
            status = 1;
            times = 0;
            cur_time = 0;
            obj1.reset();
            obj2.reset();
            obj3.reset();
        }
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE |GLUT_DEPTH);
    glutInitWindowSize(1000, 660);
    glutCreateWindow("final_work");
    status = 0;
    times = 0;
    cur_time = 0;
    initial();
    rotation = 0;
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(display);
    glutKeyboardFunc(keyinput);
    glutMainLoop();
    return 0;
}
