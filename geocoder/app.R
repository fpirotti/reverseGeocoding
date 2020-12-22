#
# This is a Shiny web application. You can run the application by clicking
# the 'Run App' button above.
#
# Find out more about building applications with Shiny here:
#
#    http://shiny.rstudio.com/
#

library(shiny)
library(readr)
library(shinyalert)
library(shinyWidgets)
library(curl)        
library(httr)
library(openxlsx)
#source("functions.R")

if(file.exists("cacheFile.rds")) {
    cacheFile<-readRDS("cacheFile.rds")
} else {
    cacheFile<-list()
}
print(file.exists("cacheFile.rds"))
keys <- list(
    google = c(
        "AIzaSyDvfSqURa60jKdXBEcL7hnwsvwEWhYgkm4",
        "AIzaSyDeAaUewDUcIriQsqOX1NNGxatX8WkbZIE",
        "AIzaSyCMV7dLRGHwkTyuOhojM03Sgh5Y3IZAGXM",
        "AIzaSyCpQoUQ9Pnz3jzCE8KpRWW1bBuzT-_a97M"
    ),
    
    bing = c(
        "AqfXHrbz_Qm4PuVk7lAPMA7ue1vuhMbJgLI-3ZwGBQ3QRIqpMcMBDx2N6OZdJYoj",
        "At2br5wQAqbVaQW6Fn9eJTTbnrMiaSkriO4lx6YdfDSGNsGI3lMYMYYjlmCbjU-5",
        "AtFnmH3kI3H_eOuB1eyxEd66fISOnuj6R7nuM_uhPAhtB7_yL1bTr0Wt4gj_ZVf8",
        "AjvjPYuoA4IgNeooKvodDLcxbVL1F8RdIxXUeYsb6PgiVapURz_PbbWvOxVKmNps"
    ),
    
    here = c("YeoIkazjfVa2WND4PUvLAjDR7VGRV_QZnfQ80TjTZSc")
)
API.list <-
    list(
        google = "https://maps.googleapis.com/maps/api/geocode/json?key=%s&region=IT&address=%s",
        bing = "http://dev.virtualearth.net/REST/v1/Locations?CountryRegion=IT&key=%s&addressLine=%s",
        bing2 = "https://www.bing.com/maps/overlay?q=%s",
        here = "https://geocode.search.hereapi.com/v1/geocode?apiKey=%s&q=%s"
        )



# Define UI for application that draws a histogram
ui <- fluidPage(

    # Application title
    titlePanel("CIRGEO reverse geocoding"),

    # Sidebar with a slider input for number of bins 
    sidebarLayout(
        sidebarPanel(
            fileInput("file1", label = h3("File input")),
            fluidRow(
                column(6, checkboxInput("header", div(title="File con prima riga con intestazione (nome colonne)",
                                                      HTML("Header<sup>?</sup>")), TRUE) ) ,
                column(6, checkboxInput("forceNonCache", 
                                     div(title="Non utilizza la cache ma obbliga nuova interrogazione da Engine selezionata", 
                                         HTML("Force<sup>?</sup>")), F) )
            ),
            numericInput("maxErrs", div(title="Maximum number of allowed errors - leave 0 for infinite",
                                        HTML("Max Errors<sup>?</sup>") ), value = 0 ),
            shiny::actionButton("process", "START"),
            downloadLink('downloadData', 'Download')
        ),

        # Show a plot of the generated distribution
        mainPanel(
            fluidRow(column(12, shinyWidgets::prettyRadioButtons(
                "type", "Engine",inline = T,
                list(
                    Google = "google",
                    Bing = "bing",
                    Here = "here"
                ), selected = "bing"
            ))),
            progressBar(id = "progBar", value = 0, total = 5000, status = "warning"),
            tableOutput("contents"),
            tableOutput("results")
        )
    )
)

# Define server logic required to draw a histogram
server <- function(input, output, session) {
    
    currentFile<-NULL
    currKey<-1
    mytable<-NULL
    
    rVals<-reactiveValues(outFile=NULL)
    
    output$contents <- renderTable({ 
        req(rVals$outFile)
        rVals$outFile
    })
    
    output$contents <- renderTable({
        
        file <- input$file1
        req(file)
        ext <- tools::file_ext(file$datapath) 
        if(file.size(file$datapath)/1000000 > 10){
            shinyWidgets::show_alert("File size is too big, please limit to 10 MB")
            return(NULL)
        }
        
        if( tolower(ext)=="zip"){
            cf<-unzip(file$datapath, list  = T)
            currentFile<<-list(name=cf[["Name"]], 
                               size=cf[["Length"]])
            unzip(file$datapath, overwrite = T)
        }
        else currentFile<<- file
        
        validate(need( is.element(tolower(ext),c("csv","zip", "txt","dat")), 
                                  "Please upload a file zip or CSV or TXT extensions. Must be a table"))
        
        mytable<<-tryCatch({ 
            read_tsv(file$datapath)
        }, error=function(e){
            return(e)
        })
        
        if(ncol(mytable)!=1){
            shinyWidgets::show_alert("Only one column with addresses please.")
            return(NULL)
        }
        
        # server
        updateProgressBar(session = session, id = "progBar", value = 0, total = nrow(mytable))
        mytable[1:10,]
    })
    
    output$downloadData <- downloadHandler(
      filename = function() {
        paste(basename(currentFile$name), '_', isolate(input$type), '_',
              format(Sys.Date(), "%Y%m%dT%s"), '.xlsx', sep='')
      },
      content = function(con) {
          outFileName<-sprintf("%s.xlsx", format(Sys.time(),  "%Y%m%dT%s"))
          openxlsx::write.xlsx(isolate(rVals$outFile), con)
      }
    )

    readURL<-function(address){
        url<-sprintf(API.list[[isolate(input$type)]], 
                     keys[[isolate(input$type)]][[currKey]],
                     URLencode(address) )
        r <- GET((url))
        if(status_code(r)!=200){
            shinyWidgets::show_alert(sprintf("Errore. Codice %s - %s<br>",
                                             status_code(r), http_status(r)$message,
                                             address )) 
        }
        
        content(r, "parsed")
    }
    
    observeEvent(input$process, {
        req(mytable) 
        resultTable<-list()
        ## each row in table
        ttrows<-nrow(mytable)
        totErrs<-0
        for(i in 1:ttrows){
            address<-mytable[i,1][[1]]
            
            if(is.null(cacheFile[[address]]) || isolate(input$forceNonCache) ){
                pC<-parseContent( readURL(address) , isolate(input$type))
                if(is.list(pC)) {
                    cacheFile[[address]]<-pC
                    resultTable[[address]]<-pC
                } else {
                    totErrs<-totErrs+1
                    if(isolate(input$maxErrs) > 0 && totErrs>isolate(input$maxErrs)){
                        shinyWidgets::show_alert(sprintf("Too many errors, aborting. 
                                                 Last error is: '%s'", pC))
                        return(NULL)
                    }
                    resultTable[[address]]<- list(
                        lat=0,
                        long=0,
                        accur=pC,
                        formattedAddress="error"
                    )
                }
                    
            } else {
                resultTable[[address]]<- cacheFile[[address]]
            } 

            
                
            
            
            updateProgressBar(session = session, id = "progBar", value = i, total = ttrows )            
            print(i)
            if(i%%100){
                saveRDS(cacheFile, "cacheFile.rds")
                #updateProgressBar(session = session, id = "progBar", value = i)
            }
        }

        
        saveRDS(cacheFile, "cacheFile.rds")
        rVals$outFile<-data.table::rbindlist(resultTable, idcol = "Original Address")
        
        
    })
}

# Run the application 
shinyApp(ui = ui, server = server)
