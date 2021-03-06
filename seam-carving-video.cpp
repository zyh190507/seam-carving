#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <vector>
#include <queue>

using namespace cv;
using namespace std;

const int INF = 100000000;
const int MAX_V = 2100000;
const int MAX_E = 13 * MAX_V;

struct flow_graph{
    int E,s,t,head,tail;
    int cap[2 * MAX_E],to[2 * MAX_E],next[2 * MAX_E],last[MAX_V],dist[MAX_V],q[MAX_V],now[MAX_V];
    
    flow_graph(){
        E = 0;
        memset(last,-1,sizeof last);
    }
    
    void clear(){
        E = 0;
        memset(last,-1,sizeof last);
    }
    
    void add_edge(int u, int v, int uv, int vu = 0){
        to[E] = v, cap[E] = uv, next[E] = last[u]; last[u] = E++;
        to[E] = u, cap[E] = vu, next[E] = last[v]; last[v] = E++;
    }
	
    bool bfs(){
        memset(dist,-1,sizeof dist);
        head = tail = 0;
		
        q[tail] = t; ++tail;
        dist[t] = 0;
		
        while(head < tail){
            int v = q[head]; ++head;
            
            for(int e = last[v];e != -1;e = next[e]){
                if(cap[e^1] > 0 && dist[to[e]] == -1){
                    q[tail] = to[e]; ++tail;
                    dist[to[e]] = dist[v]+1;
                }
            }
        }
        
        return dist[s] != -1;
    }
	
    int dfs(int v, int f){
        if(v == t) return f;
		
        for(int &e = now[v];e != -1;e = next[e]){
            if(cap[e] > 0 && dist[to[e]] == dist[v]-1){
                int ret = dfs(to[e],min(f,cap[e]));
				
                if(ret > 0){
                    cap[e] -= ret;
                    cap[e^1] += ret;
                    return ret;
                }
            }
        }
		
        return 0;
    }
	
    void max_flow(int source, int sink, int V){
        s = source; t = sink;
        int df;
		
        while(bfs()){
            for(int i = 0;i < V;++i) now[i] = last[i];
			
            while(true){
                df = dfs(s,INT_MAX);
                if(df == 0) break;
            }
        }
    }
}G;

bool inS[MAX_V];

vector<int> get_min_cut(int V){
	memset(inS,false,sizeof inS);

	queue<int> Q;
	Q.push(V - 2);
	inS[V - 2] = true;

	while(!Q.empty()){
		int cur = Q.front();
		Q.pop();

		for(int e = G.last[cur];e != -1;e = G.next[e]){
			if(e % 2 == 0 && G.cap[e] > 0 && !inS[ G.to[e] ]){
				inS[ G.to[e] ] = true;
				Q.push(G.to[e]);
			}
		}
	}

	vector<int> ret;

	for(int e = 0;e < G.E;e += 2)
		if(inS[ G.to[e ^ 1] ] && !inS[ G.to[e] ])
			ret.push_back(e);

	return ret;
}

#define MAXT 500

int T = 0,Y0,X0;
Mat frame[MAXT];
int Yin,Xin;
Mat frameIn[MAXT],graysIn[MAXT];
Mat frameOut[MAXT];
Mat seam[MAXT];

int id(int t, int y, int x){
	return t * Yin * Xin + y * Xin + x;
}

int get(Mat &I, int y, int x){
    return (int)I.at<uchar>(y,x);
}

void remove_horizontal_seam(){
	for(int t = 0;t < T;++t)
		cvtColor(frameIn[t],graysIn[t],CV_BGR2GRAY);

	G.clear();
	int V = 2 + T * Yin * Xin;
	cout << "V = " << V << endl;

	for(int t = 0;t < T;++t){

		for(int y = 0;y < Yin;++y){
			for(int x = 0;x < Xin;++x){
				// (Y,X) edges

				if(y + 1 < Yin){
					int LR;

					if(y > 0)
						LR = abs(get(graysIn[t],y + 1,x) - get(graysIn[t],y - 1,x));
					else
						LR = abs(get(graysIn[t],y + 1,x) - get(graysIn[t],y,x));

					int u = id(t,y,x),v = id(t,y + 1,x);
					G.add_edge(u,v,LR);
					G.add_edge(v,u,INF);
				}
				
				if(x + 1 < Xin){
					int posLU = 0,negLU = 0;

					if(x > 0 && y > 0)
						posLU = abs(get(graysIn[t],y,x - 1) - get(graysIn[t],y - 1,x));
					else if(x > 0)
						posLU = abs(get(graysIn[t],y,x - 1) - get(graysIn[t],y,x));
					else if(y > 0)
						posLU = abs(get(graysIn[t],y,x) - get(graysIn[t],y - 1,x));

					if(x + 1 < Xin && y > 0)
						negLU = abs(get(graysIn[t],y,x + 1) - get(graysIn[t],y - 1,x));
					else if(x + 1 < Xin)
						negLU = abs(get(graysIn[t],y,x + 1) - get(graysIn[t],y,x));
					else if(y > 0)
						negLU = abs(get(graysIn[t],y,x) - get(graysIn[t],y - 1,x));

					int u = id(t,y,x),v = id(t,y,x + 1);
					G.add_edge(u,v,negLU);
					G.add_edge(v,u,posLU);
				}
				
				if(x > 0 && y > 0)
					G.add_edge(id(t,y,x),id(t,y - 1,x - 1),INF);

				if(y > 0 && x + 1 < Xin)
					G.add_edge(id(t,y,x),id(t,y - 1,x + 1),INF);

				// (Y,T) edges

				if(t + 1 < T){
					int LR;

					if(t > 0)
						LR = abs(get(graysIn[t + 1],y,x) - get(graysIn[t - 1],y,x));
					else
						LR = abs(get(graysIn[t + 1],y,x) - get(graysIn[t],y,x));

					int u = id(t,y,x),v = id(t + 1,y,x);
					G.add_edge(u,v,LR);
					G.add_edge(u,v,INF);
				}

				if(y + 1 < Yin){
					int posLU = 0,negLU = 0;

					if(y > 0 && t > 0)
						posLU = abs(get(graysIn[t],y - 1,x) - get(graysIn[t - 1],y,x));
					else if(y > 0)
						posLU = abs(get(graysIn[t],y - 1,x) - get(graysIn[t],y,x));
					else if(t > 0)
						posLU = abs(get(graysIn[t],y,x) - get(graysIn[t - 1],y,x));

					if(y + 1 < Yin && t > 0)
						negLU = abs(get(graysIn[t],y + 1,x) - get(graysIn[t - 1],y,x));
					else if(y + 1 < Yin)
						negLU = abs(get(graysIn[t],y + 1,x) - get(graysIn[t],y,x));
					else if(t > 0)
						negLU = abs(get(graysIn[t],y,x) - get(graysIn[t - 1],y,x));

					int u = id(t,y,x),v = id(t,y + 1,x);
					G.add_edge(u,v,negLU);
					G.add_edge(v,u,posLU);
				}
				
				if(t > 0 && y > 0)
					G.add_edge(id(t,y,x),id(t - 1,y - 1,x),INF);

				if(t > 0 && y + 1 < Yin)
					G.add_edge(id(t,y,x),id(t - 1,y + 1,x),INF);
			}
		}
	}

	for(int t = 0;t < T;++t){
		for(int x = 0;x < Xin;++x)
			G.add_edge(V - 2,id(t,0,x),INF);

		for(int x = 0;x < Xin;++x)
			G.add_edge(id(t,Yin - 1,x),V - 1,INF);
	}

    cout << "E = " << G.E / 2 << endl;
    cout << "max flow" << endl;
	G.max_flow(V - 2,V - 1,V);
	
	for(int t = 0;t < T;++t)
		seam[t] = frameIn[t].clone();

	vector<int> cut = get_min_cut(V);
	sort(cut.begin(),cut.end());

	for(int t = 0;t < T;++t)
		frameOut[t] = Mat_<Vec3b>(Yin - 1,Xin);

	for(int i = 0;i < cut.size();i++){
		int e = cut[i];
		int u = G.to[e ^ 1],v = G.to[e];

		int t1 = u / (Yin * Xin),y1 = u % (Yin * Xin) / Xin,x1 = u % Xin;
		int t2 = v / (Yin * Xin),y2 = v % (Yin * Xin) / Xin,x2 = v % Xin;

		if(t1 == t2 && x1 == x2){
			seam[t1].at<Vec3b>(y1,x1) = Vec3b(0,0,255);

			for(int y = 0;y < Yin;++y){
				if(y < y1)
					frameOut[t1].at<Vec3b>(y,x1) = frameIn[t1].at<Vec3b>(y,x1);
				else
					frameOut[t1].at<Vec3b>(y - 1,x1) = frameIn[t1].at<Vec3b>(y,x1);
			}
		}
	}

    for(int t = 0;t < T;++t)
        frameIn[t] = frameOut[t].clone();

    // show seams
	/*
    for(int t = 0;t < T;++t){
		imshow("original",frame[t]);
		imshow("seam",seam[t]);
		imshow("result",frameOut[t]);
		waitKey(0);
	}

    destroyWindow("seam");
    */

    // save seams
    VideoWriter out("seams-horizontal.avi",CV_FOURCC('M','J','P','G'),10, Size(Xin,Yin),true);

    for(int t = 0;t < T;++t)
        out.write(seam[t]);

	--Yin;
}

void remove_vertical_seam(){
	for(int t = 0;t < T;++t)
		cvtColor(frameIn[t],graysIn[t],CV_BGR2GRAY);

	G.clear();
	int V = 2 + T * Yin * Xin;
	cout << "V = " << V << endl;

	for(int t = 0;t < T;++t){

		for(int y = 0;y < Yin;++y){
			for(int x = 0;x < Xin;++x){
				// (Y,X) edges

				if(x + 1 < Xin){
					int LR;

					if(x > 0)
						LR = abs(get(graysIn[t],y,x + 1) - get(graysIn[t],y,x - 1));
					else
						LR = abs(get(graysIn[t],y,x + 1) - get(graysIn[t],y,x));

					int u = id(t,y,x),v = id(t,y,x + 1);
					G.add_edge(u,v,LR);
					G.add_edge(v,u,INF);
				}
				
				if(y + 1 < Yin){
					int posLU = 0,negLU = 0;

					if(y > 0 && x > 0)
						posLU = abs(get(graysIn[t],y - 1,x) - get(graysIn[t],y,x - 1));
					else if(y > 0)
						posLU = abs(get(graysIn[t],y - 1,x) - get(graysIn[t],y,x));
					else if(x > 0)
						posLU = abs(get(graysIn[t],y,x) - get(graysIn[t],y,x - 1));

					if(y + 1 < Yin && x > 0)
						negLU = abs(get(graysIn[t],y + 1,x) - get(graysIn[t],y,x - 1));
					else if(y + 1 < Yin)
						negLU = abs(get(graysIn[t],y + 1,x) - get(graysIn[t],y,x));
					else if(x > 0)
						negLU = abs(get(graysIn[t],y,x) - get(graysIn[t],y,x - 1));

					int u = id(t,y,x),v = id(t,y + 1,x);
					G.add_edge(u,v,negLU);
					G.add_edge(v,u,posLU);
				}
				
				if(x > 0 && y > 0)
					G.add_edge(id(t,y,x),id(t,y - 1,x - 1),INF);

				if(x > 0 && y + 1 < Yin)
					G.add_edge(id(t,y,x),id(t,y + 1,x - 1),INF);

				// (X,T) edges

				if(t + 1 < T){
					int LR;

					if(t > 0)
						LR = abs(get(graysIn[t + 1],y,x) - get(graysIn[t - 1],y,x));
					else
						LR = abs(get(graysIn[t + 1],y,x) - get(graysIn[t],y,x));

					int u = id(t,y,x),v = id(t + 1,y,x);
					G.add_edge(u,v,LR);
					G.add_edge(u,v,INF);
				}

				if(x + 1 < Xin){
					int posLU = 0,negLU = 0;

					if(x > 0 && t > 0)
						posLU = abs(get(graysIn[t],y,x - 1) - get(graysIn[t - 1],y,x));
					else if(x > 0)
						posLU = abs(get(graysIn[t],y,x - 1) - get(graysIn[t],y,x));
					else if(t > 0)
						posLU = abs(get(graysIn[t],y,x) - get(graysIn[t - 1],y,x));

					if(x + 1 < Xin && t > 0)
						negLU = abs(get(graysIn[t],y,x + 1) - get(graysIn[t - 1],y,x));
					else if(x + 1 < Xin)
						negLU = abs(get(graysIn[t],y,x + 1) - get(graysIn[t],y,x));
					else if(t > 0)
						negLU = abs(get(graysIn[t],y,x) - get(graysIn[t - 1],y,x));

					int u = id(t,y,x),v = id(t,y,x + 1);
					G.add_edge(u,v,negLU);
					G.add_edge(v,u,posLU);
				}
				
				if(t > 0 && x > 0)
					G.add_edge(id(t,y,x),id(t - 1,y,x - 1),INF);

				if(t > 0 && x + 1 < Xin)
					G.add_edge(id(t,y,x),id(t - 1,y,x + 1),INF);
			}
		}
	}

	for(int t = 0;t < T;++t){
		for(int y = 0;y < Yin;++y)
			G.add_edge(V - 2,id(t,y,0),INF);

		for(int y = 0;y < Yin;++y)
			G.add_edge(id(t,y,Xin - 1),V - 1,INF);
	}

    cout << "E = " << G.E / 2 << endl;
    cout << "max flow" << endl;
	G.max_flow(V - 2,V - 1,V);

	
	for(int t = 0;t < T;++t)
		seam[t] = frameIn[t].clone();

	vector<int> cut = get_min_cut(V);
	sort(cut.begin(),cut.end());

	for(int t = 0;t < T;++t)
		frameOut[t] = Mat_<Vec3b>(Yin,Xin - 1);

	for(int i = 0;i < cut.size();i++){
		int e = cut[i];
		int u = G.to[e ^ 1],v = G.to[e];

		int t1 = u / (Yin * Xin),y1 = u % (Yin * Xin) / Xin,x1 = u % Xin;
		int t2 = v / (Yin * Xin),y2 = v % (Yin * Xin) / Xin,x2 = v % Xin;

		if(t1 == t2 && y1 == y2){
			seam[t1].at<Vec3b>(y1,x1) = Vec3b(0,0,255);

			for(int x = 0;x < Xin;++x){
				if(x < x1)
					frameOut[t1].at<Vec3b>(y1,x) = frameIn[t1].at<Vec3b>(y1,x);
				else
					frameOut[t1].at<Vec3b>(y1,x - 1) = frameIn[t1].at<Vec3b>(y1,x);
			}
		}
	}

    for(int t = 0;t < T;++t)
        frameIn[t] = frameOut[t].clone();

    // show seams
    /*
	for(int t = 0;t < T;++t){
		imshow("original",frame[t]);
		imshow("seam",seam[t]);
		imshow("result",frameOut[t]);
		waitKey(0);
	}

    destroyWindow("seam");
    */

    // save seams
    VideoWriter out("seams-vertical.avi",CV_FOURCC('M','J','P','G'),10, Size(Xin,Yin),true);

    for(int t = 0;t < T;++t)
        out.write(seam[t]);

    --Xin;
}

String to_string(int n){
    if(n == 0) return "0";
    String ret;

    while(n > 0){
        ret = (char)('0' + n % 10) + ret;
        n /= 10;
    }

    return ret;
}

void write(string file){
    VideoWriter out(file,CV_FOURCC('M','J','P','G'),10, Size(Xin,Yin),true);

    for(int t = 0;t < T;++t)
        out.write(frameIn[t]);
}

void reduce(int dy, int dx){
	cout << "reduce : " << dy << " " << dx << endl;

	for(int i = 0;i < T;++i)
		frameIn[i] = frame[i].clone();
	Yin = Y0; Xin = X0;

	for(int i = 0;i < dy;++i){
		cout << "\nHorizontal seam " << i << endl;
		remove_horizontal_seam();
        write("result-horizontal-" + to_string(i) + ".avi");
	}

	for(int i = 0;i < dx;++i){
		cout << "\nVertical seam " << i << endl;
		remove_vertical_seam();
        write("result-vertical-" + to_string(i) + ".avi");
	}

    // show results
    /*
	for(int t = 0;t < T;++t){
		cout << "show result " << t << endl;
		imshow("original",frame[t]);
		imshow("result",frameOut[t]);
		waitKey(0);
	}
    */

    // save results
    write("result.avi");
}

int main(){
    freopen("../input-video.txt","r",stdin);

    string file;
    cin >> file;

    int dy,dx;

    cin >> dy >> dx;

    VideoCapture capture;
    capture.open(file);

    while(true){
    	capture >> frame[T];

    	if(frame[T].empty())
    		break;
    	
    	++T;
    }

    int maxT;
    cin >> maxT;

    T = min(T,maxT);
    cout << "T = " << T << endl;

    Y0 = frame[0].rows;
    X0 = frame[0].cols;

    cout << "Y0 = " << Y0 << ", X0 = " << X0 << endl;

    reduce(dy,dx);

    return 0;
}
