#include "PartnerCsv.h"
#include <map>
#include <vector>
#include <algorithm>
namespace erp {
namespace {
std::string trim(std::string value) {
    for(const auto &space:{std::string("\xE3\x80\x80"),std::string("\xC2\xA0")}) {
        size_t pos=0;while((pos=value.find(space,pos))!=std::string::npos){value.replace(pos,space.size()," ");++pos;}
    }
    auto first=value.find_first_not_of(" \t\r\n");if(first==std::string::npos)return "";
    return value.substr(first,value.find_last_not_of(" \t\r\n")-first+1);
}
size_t characters(const std::string &value) {size_t count=0;for(unsigned char c:value)if((c&0xc0)!=0x80)++count;return count;}
struct Record {size_t line;std::vector<std::string> cells;};
std::vector<Record> parse(const std::string &input) {
    using business::require;
    require(input.size()<=5*1024*1024,"CSV 文件不能超过5MB");
    std::string csv=input;if(csv.compare(0,3,"\xEF\xBB\xBF")==0)csv.erase(0,3);
    std::vector<Record> records;std::vector<std::string> cells;std::string cell;
    bool quoted=false,closed=false;size_t line=1,start=1;
    auto finish=[&](){cells.push_back(cell);bool empty=true;for(auto &value:cells)if(!trim(value).empty())empty=false;if(!empty)records.push_back({start,cells});cells.clear();cell.clear();closed=false;require(records.size()<=5001,"每次最多导入5000条数据");};
    for(size_t i=0;i<csv.size();++i){char c=csv[i];
        if(quoted){if(c=='"'){if(i+1<csv.size() && csv[i+1]=='"'){cell+='"';++i;}else{quoted=false;closed=true;}}else{cell+=c;if(c=='\n')++line;}continue;}
        if(c==','){cells.push_back(cell);cell.clear();closed=false;continue;}
        if(c=='\r' || c=='\n'){if(c=='\r' && i+1<csv.size() && csv[i+1]=='\n')++i;finish();++line;start=line;continue;}
        if(c=='"'){require(!closed && cell.empty(),"CSV 第"+std::to_string(line)+"行引号格式错误");quoted=true;continue;}
        if(closed){require(c==' ' || c=='\t',"CSV 第"+std::to_string(line)+"行引号后的内容错误");continue;}
        cell+=c;
    }
    require(!quoted,"CSV 引号未闭合，请检查文件格式");
    if(!cell.empty() || !cells.empty() || closed)finish();
    require(!records.empty(),"CSV 内容为空");return records;
}
std::string headerKey(std::string name) {
    name=trim(name);std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return c>='A' && c<='Z'?char(c-'A'+'a'):char(c);});
    static const std::map<std::string,std::string> aliases={{"名称","name"},{"供应商名称","name"},{"客户名称","name"},{"供应商","name"},{"客户","name"},{"联系人","contact"},{"电话","phone"},{"联系电话","phone"},{"手机号","phone"},{"邮箱","email"},{"电子邮箱","email"},{"地址","address"},{"状态","status"},{"备注","remark"}};
    auto found=aliases.find(name);return found==aliases.end()?name:found->second;
}
}
void importPartners(const std::string &table,const std::string &csv,int64_t user,business::Callback callback) {
    std::vector<Record> records;std::map<std::string,size_t> columns;
    try {
        business::require(table=="suppliers" || table=="customers","不支持的导入类型");records=parse(csv);
        for(size_t i=0;i<records[0].cells.size();++i){auto key=headerKey(records[0].cells[i]);business::require(!columns.count(key),"CSV 表头存在重复字段："+key);columns[key]=i;}
        business::require(columns.count("name"),"CSV 表头必须包含名称（name、供应商名称或客户名称）");
        business::require(records.size()>1,"CSV 文件没有数据行");
    }catch(const std::exception &e){callback(ErrorCode::BAD_REQUEST,e.what(),Json::Value());return;}
    business::transaction(std::move(callback),[=](const business::Tx &tx){
        using namespace business;actor(tx,user);
        require(!tx->execSqlSync("SELECT ur.user_id FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=? AND r.code='admin'",user).empty(),"仅管理员可导入基础资料",ErrorCode::FORBIDDEN);
        // Serialize imports of the same kind, so simultaneous uploads cannot insert the same name.
        tx->execSqlSync("INSERT INTO master_import_locks (kind) VALUES (?) ON DUPLICATE KEY UPDATE kind=kind",table);
        tx->execSqlSync("SELECT kind FROM master_import_locks WHERE kind=? FOR UPDATE",table);
        int added=0,skipped=0,failed=0;Json::Value issues(Json::arrayValue);
        const std::map<std::string,size_t> limits={{"name",128},{"contact",64},{"phone",32},{"email",128},{"address",255},{"remark",255}};
        auto note=[&](const Record &record,const std::string &name,const std::string &kind,const std::string &message){if(issues.size()>=100)return;Json::Value item;item["row"]=Json::UInt64(record.line);item["name"]=name;item["type"]=kind;item["message"]=message;issues.append(item);};
        for(size_t index=1;index<records.size();++index){const auto &record=records[index];std::map<std::string,std::string> values;std::string problem;
            if(record.cells.size()!=records[0].cells.size()){++failed;note(record,"","failed","列数与表头不一致");continue;}
            for(auto field:limits){auto position=columns.find(field.first);values[field.first]=position==columns.end()?"":trim(record.cells[position->second]);if(characters(values[field.first])>field.second)problem=field.first+" 超过允许长度";}
            if(values["name"].empty())problem="名称不能为空";
            int status=1;auto position=columns.find("status");auto state=position==columns.end()?"":trim(record.cells[position->second]);
            if(state.empty() || state=="1" || state=="正常" || state=="合作中" || state=="启用")status=1;
            else if(state=="0" || state=="停用" || state=="禁用")status=0;
            else problem="状态应为0或1（也支持正常、合作中、启用、停用）";
            if(!problem.empty()){++failed;note(record,values["name"],"failed",problem);continue;}
            auto existing=tx->execSqlSync("SELECT id FROM "+table+" WHERE LOWER(TRIM(REPLACE(REPLACE(REPLACE(name,CHAR(9),' '),'　',' '),' ',' '))) COLLATE utf8mb4_unicode_ci = LOWER(?) COLLATE utf8mb4_unicode_ci LIMIT 1 FOR UPDATE",values["name"]);
            if(!existing.empty()){++skipped;note(record,values["name"],"duplicate","名称已存在，已跳过，原资料未改动");continue;}
            auto inserted=tx->execSqlSync("INSERT INTO "+table+" (name,contact,phone,email,address,status,remark) VALUES (?,?,?,?,?,?,?)",values["name"],values["contact"],values["phone"],values["email"],values["address"],status,values["remark"]);
            event(tx,table,inserted.insertId(),"import",user,-1,status);++added;
        }
        Outcome out;out.data["success"]=added;out.data["skipped"]=skipped;out.data["failed"]=failed;out.data["total"]=static_cast<int>(records.size()-1);out.data["issues"]=issues;out.data["issues_truncated"]=skipped+failed>static_cast<int>(issues.size());out.message="导入完成：新增"+std::to_string(added)+"条，重复跳过"+std::to_string(skipped)+"条，失败"+std::to_string(failed)+"条";return out;
    });
}
}
